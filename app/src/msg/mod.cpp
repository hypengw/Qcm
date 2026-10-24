module;
#include "Qcm/msg/store.moc.h"
#include "Qcm/message/entity_store.hpp"
#include "core/log.h"
module qcm;
import :msg;
import :global;
import qcm.log;

auto rstd::Impl<rstd::convert::From<qcm::enums::ItemType>,
                qcm::msg::model::ItemTypeGadget::ItemType>::from(in_t t) -> out_t {
    return static_cast<out_t>(static_cast<int>(t));
}

namespace qcm
{
static store::Key cacheKey(model::ItemId id) {
    return store::canonical({ int(id.type()), id.id() });
}
AppStore::AppStore(QObject* parent)
    : QObject(parent),
      albums(reader<model::Album>(store::Album)),
      songs(reader<model::Song>(store::Song)),
      artists(reader<model::Artist>(store::Artist)),
      mixes(reader<model::Mix>(store::Mix)) {}
AppStore::~AppStore() = default;
AppStore* AppStore::instance() {
    static auto value = GlobalStatic::instance()->add<AppStore>(
        "store", new AppStore(nullptr), [](AppStore* pointer) {
            delete pointer;
        });
    return value;
}
AppStore* AppStore::create(QQmlEngine*, QJSEngine*) {
    auto value = instance();
    QJSEngine::setObjectOwnership(value, QJSEngine::CppOwnership);
    return value;
}
const store::Entry* AppStore::find(model::ItemId id) const { return m_data.find(cacheKey(id)); }
void                AppStore::pin(model::ItemId id) {
    if (id.valid()) m_data.pin(cacheKey(id));
}
void AppStore::unpin(model::ItemId id) {
    if (id.valid()) m_data.unpin(cacheKey(id));
    if (! m_collectScheduled) {
        m_collectScheduled = true;
        QTimer::singleShot(0, this, [this] {
            m_collectScheduled = false;
            collect();
        });
    }
}
int  AppStore::status(model::ItemId id) const { return int(m_data.status(cacheKey(id))); }
void AppStore::markStale() {
    enqueue([this] {
        publish(m_data.markStale());
    });
}
StoreSubscription AppStore::subscribe(std::function<void(model::ItemId, quint64)> callback) {
    auto connection = connect(this, &AppStore::changed, this, std::move(callback));
    return StoreSubscription([connection] {
        QObject::disconnect(connection);
    });
}
StoreSubscription AppStore::subscribe(model::ItemId id, std::function<void(quint64)> callback) {
    const auto key   = cacheKey(id);
    const auto token = ++m_observerSerial;
    m_observers[key].insert(token, std::move(callback));
    return StoreSubscription([weak = QPointer<AppStore>(this), key, token] {
        if (! weak) return;
        auto found = weak->m_observers.find(key);
        if (found == weak->m_observers.end()) return;
        found->remove(token);
        if (found->isEmpty()) weak->m_observers.erase(found);
    });
}
void AppStore::publish(const QList<store::Change>& changes) {
    for (const auto& change : changes) {
        changed(model::ItemId(static_cast<enums::ItemType>(change.key.type), change.key.id),
                change.fields);
        const auto observers = m_observers.value(change.key).keys();
        for (auto token : observers) {
            auto found = m_observers.constFind(change.key);
            if (found == m_observers.cend()) break;
            auto callback = found->value(token);
            if (callback) callback(change.fields);
        }
    }
}
void AppStore::enqueue(std::function<void()> operation) {
    Q_ASSERT(QThread::currentThread() == thread());
    m_pending.push_back(std::move(operation));
    if (m_publishing) return;
    m_publishing = true;
    while (! m_pending.isEmpty()) {
        auto operation = m_pending.takeFirst();
        operation();
    }
    m_publishing = false;
}
void AppStore::deliver(msg::entity::EntityBatch batch, std::function<void(QString)> completed) {
    const auto generation = m_deliveryGeneration;
    enqueue([this, generation, batch = std::move(batch), completed = std::move(completed)] {
        if (generation != m_deliveryGeneration) {
            completed(QStringLiteral("entity store reset"));
            return;
        }
        const auto scope  = m_data.scopeRevision();
        auto       result = m_data.apply(batch);
        if (result) {
            publish(result.changes);
            for (auto key : result.stateChanges)
                stateChanged(model::ItemId(static_cast<enums::ItemType>(key.type), key.id));
            if (scope != m_data.scopeRevision()) scopeChanged();
        }
        completed(result.error);
    });
}
void AppStore::reset() {
    ++m_deliveryGeneration;
    enqueue([this] {
        publish(m_data.clear());
    });
}
void AppStore::collect() {
    enqueue([this] {
        publish(m_data.collect());
    });
}
EntityView* AppStore::item(model::ItemId id) {
    if (auto existing = m_views.value(id)) return existing;
    auto view = new EntityView;
    view->setItemId(id);
    QJSEngine::setObjectOwnership(view, QJSEngine::JavaScriptOwnership);
    m_views.insert(id, view);
    connect(view, &QObject::destroyed, this, [this, id, view] {
        if (m_views.value(id).isNull() || m_views.value(id) == view) m_views.remove(id);
    });
    return view;
}
EntityView::EntityView(QObject* parent): QObject(parent), m_store(AppStore::instance()) {}
EntityView::~EntityView() {
    m_subscription.reset();
    if (m_store) m_store->unpin(m_id);
}
model::ItemId EntityView::itemId() const { return m_id; }
void          EntityView::setItemId(model::ItemId id) {
    if (m_id == id) return;
    m_subscription.reset();
    if (m_store) m_store->unpin(m_id);
    m_id = id;
    if (m_store && id.valid()) {
        m_store->pin(id);
        m_subscription = m_store->subscribe(id, [this](quint64) {
            changed();
        });
    }
    changed();
}
QList<model::Artist> AppStore::artistsFor(model::ItemId id) const {
    QList<model::Artist> values;
    if (auto entry = find(id))
        for (auto target : entry->artists) {
            if (auto item = artists.store_query(target))
                values.push_back(*item);
            else {
                model::Artist value;
                value.setId_proto(target);
                values.push_back(value);
            }
        }
    return values;
}
model::Album AppStore::albumFor(model::ItemId id) const {
    if (auto entry = find(id); entry && entry->album)
        if (auto value = albums.store_query(*entry->album)) return *value;
    return {};
}
msg::entity::ItemState AppStore::stateFor(model::ItemId id) const {
    msg::entity::ItemState value;
    bool                   favorite = false;
    if (auto entry = find(id)) favorite = entry->favorite.value_or(false);
    value.setFavorite(favorite);
    return value;
}
QList<model::Artist> EntityView::artists() const {
    return m_store ? m_store->artistsFor(m_id) : QList<model::Artist>();
}
model::Album EntityView::album() const {
    return m_store ? m_store->albumFor(m_id) : model::Album();
}
msg::entity::ItemState EntityView::state() const {
    return m_store ? m_store->stateFor(m_id) : msg::entity::ItemState();
}
EntityView::Status EntityView::status() const {
    return m_store ? Status(m_store->status(m_id)) : Missing;
}
bool EntityView::loaded() const { return m_store && m_store->find(m_id); }
bool EntityView::artistsKnown() const {
    auto e = m_store ? m_store->find(m_id) : nullptr;
    return e && e->known(store::Field::Artists);
}
bool EntityView::albumKnown() const {
    auto e = m_store ? m_store->find(m_id) : nullptr;
    return e && e->known(store::Field::Album);
}
bool EntityView::stateKnown() const {
    auto e = m_store ? m_store->find(m_id) : nullptr;
    return e && e->known(store::Field::Favorite);
}
quint64 EntityView::nullFields() const {
    auto entry = m_store ? m_store->find(m_id) : nullptr;
    return entry ? entry->nulls : 0;
}
quint64 EntityView::knownFields() const {
    quint64 mask = 0;
    if (m_store)
        if (auto entry = m_store->find(m_id))
            for (unsigned i = 0; i < unsigned(store::Field::Count); ++i)
                if (entry->versions[i]) mask |= quint64(1) << i;
    return mask;
}
} // namespace qcm
#include "Qcm/msg/store.moc.cpp"
