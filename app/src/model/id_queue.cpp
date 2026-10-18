module;
#include "Qcm/model/id_queue.moc.h"
module qcm;
import :model.id_queue;
import :global;
import :app;
import :query.play;

namespace qcm::model
{
IdQueue::IdQueue(QObject* parent)
    : QAbstractListModel(parent), m_current_index(-1), m_name("Queue") {
    connect(this, &IdQueue::currentIndexChanged, this, &IdQueue::currentIdChanged);
}
IdQueue::~IdQueue() {}

auto IdQueue::rowCount(const QModelIndex&) const -> int { return m_queue.size(); }
auto IdQueue::data(const QModelIndex& index, int) const -> QVariant {
    if (! index.isValid() || index.row() >= (int)m_queue.size()) return {};
    return QVariant::fromValue(m_queue.at(index.row()).id);
}
auto IdQueue::name() const -> const QString& { return m_name; }
void IdQueue::setName(QStringView name) { m_name = name.toString(); }

auto IdQueue::options() const -> Options { return m_opts; }
void IdQueue::setOptions(Options opts) { m_opts = opts; }
auto IdQueue::currentId() const -> model::ItemId {
    auto cur = currentIndex();
    if (cur < 0 || cur >= rowCount()) {
        return {};
    }
    return m_queue.at(cur).id;
}

auto IdQueue::currentExtra() const -> std::optional<QVariant> {
    auto cur = currentIndex();
    if (cur < 0 || cur > rowCount()) {
        return std::nullopt;
    }
    return m_queue.at(cur).extra;
}

auto IdQueue::currentOrFirstExtra() const -> std::optional<QVariant> {
    auto cur = currentIndex();
    if (cur < 0 || cur > rowCount()) {
        if (rowCount() > 0) {
            cur = 0;
        } else {
            return std::nullopt;
        }
    }
    return m_queue.at(cur).extra;
}

auto IdQueue::currentIndex() const -> qint32 { return m_current_index.value(); }
void IdQueue::setCurrentIndex(qint32 idx) { m_current_index = idx; }
auto IdQueue::bindableCurrentIndex() -> QBindable<qint32> { return &m_current_index; }
void IdQueue::setCurrentIndex(const model::ItemId& id) {
    qint32 i = 0;
    for (auto& el : m_queue) {
        if (el.id == id) {
            setCurrentIndex(i);
            break;
        }
        i++;
    }
}

auto IdQueue::contains(const model::ItemId& id) const -> bool {
    auto hash = std::hash<model::ItemId> {}(id);
    return m_set.contains(hash);
}

auto IdQueue::insert(qint32 pos, std::span<const model::ItemId> ids) -> int {
    if (pos > (int)m_queue.size()) return 0;
    std::vector<usize> hashes;
    std::vector<Item>  insert_ids;
    for (auto& el : ids) {
        auto hash = std::hash<model::ItemId>()(el);
        if (! m_set.contains(hash)) {
            hashes.push_back(hash);
            insert_ids.emplace_back(el);
        }
    }

    if (hashes.empty()) return 0;
    beginInsertRows({}, pos, pos + hashes.size() - 1);

    m_queue.insert(m_queue.begin() + pos, insert_ids.begin(), insert_ids.end());
    m_set.insert(hashes.begin(), hashes.end());

    endInsertRows();
    return hashes.size();
}

auto IdQueue::insert(qint32 pos, std::span<const Item> items) -> int {
    if (pos > (int)m_queue.size()) return 0;
    std::vector<usize> hashes;
    std::vector<Item>  insert_items;
    for (auto& el : items) {
        auto hash = std::hash<model::ItemId>()(el.id);
        if (! m_set.contains(hash)) {
            hashes.push_back(hash);
            insert_items.emplace_back(el);
        }
    }

    if (hashes.empty()) return 0;
    beginInsertRows({}, pos, pos + hashes.size() - 1);

    m_queue.insert(m_queue.begin() + pos, insert_items.begin(), insert_items.end());
    m_set.insert(hashes.begin(), hashes.end());

    endInsertRows();
    return hashes.size();
}

void IdQueue::remove(const model::ItemId& id) {
    if (auto it = std::find_if(m_queue.begin(),
                               m_queue.end(),
                               [&id](const auto& el) {
                                   return el.id == id;
                               });
        it != m_queue.end()) {
        removeRow(std::distance(m_queue.begin(), it));
    }
}
bool IdQueue::removeRows(int first, int count, const QModelIndex& parent) {
    if (count <= 0) return false;
    count     = std::max(count, 1) - 1;
    auto last = std::min(first + count, std::max<int>(m_queue.size(), 1) - 1);

    beginRemoveRows(parent, first, last);
    auto begin = m_queue.begin() + first;
    auto end   = begin + count + 1;
    {
        auto it = begin;
        while (it != end) m_set.erase(std::hash<model::ItemId> {}((it++)->id));
    }
    m_queue.erase(begin, end);
    endRemoveRows();

    if (m_current_index > last) {
        setCurrentIndex(m_current_index - (count + 1));
    } else if (m_current_index <= last && m_current_index >= first) {
        setCurrentIndex(-1);
    } else {
    }
    return true;
}
bool IdQueue::moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
                       const QModelIndex& destinationParent, int destinationChild) {
    if (sourceRow < 0 || sourceRow + count - 1 >= rowCount(sourceParent) || destinationChild < 0 ||
        destinationChild > rowCount(destinationParent) || sourceRow == destinationChild - 1 ||
        count <= 0 || sourceParent.isValid() || destinationParent.isValid()) {
        return false;
    }
    if (! beginMoveRows(
            QModelIndex(), sourceRow, sourceRow + count - 1, QModelIndex(), destinationChild))
        return false;
    {
        auto it  = m_queue.begin();
        auto src = it + sourceRow;
        auto dst = it + destinationChild;
        if (src > dst) {
            std::rotate(dst, src, src + count);
        } else {
            std::rotate(src, src + count, dst);
        }
    }
    endMoveRows();
    return true;
}

bool IdQueue::move(qint32 src, qint32 dst, qint32 count) {
    auto p = index(-1);
    return moveRows(p, src, count, p, dst);
}

DynamicIdQueue::DynamicIdQueue(qint64 queue_id, const QString& name,
                               const QList<msg::model::QueueConfigField>& configuration,
                               bool can_restart, const QString& restart_label, QObject* parent)
    : IdQueue(parent),
      m_queue_id(queue_id),
      m_query(new QueueNextQuery(this)),
      m_open_query(new QueueNextQuery(this)),
      m_can_restart(can_restart),
      m_restart_label(restart_label),
      m_configuration(configuration) {
    setName(name);
    setOptions(SupportPrev | Dynamic);
    connect(static_cast<QueueNextQuery*>(m_query),
            &QueueNextQuery::statusChanged,
            this,
            &DynamicIdQueue::on_query_finished);
    connect(static_cast<QueueNextQuery*>(m_open_query),
            &QueueNextQuery::statusChanged,
            this,
            &DynamicIdQueue::on_open_finished);
    connect(this, &IdQueue::requestNext, this, [this] {
        m_suppress_prefetch = false;
        on_request_next();
    });
    connect(this, &IdQueue::currentIndexChanged, this, &DynamicIdQueue::on_current_index_changed);
    connect(App::instance()->backend(), &Backend::disconnected, this, [this] {
        if (m_session.id_proto().isEmpty()) return;
        m_session_expired = true;
        sessionChanged();
    });
}

DynamicIdQueue::~DynamicIdQueue() { close(m_session.id_proto()); }
auto DynamicIdQueue::queueId() const -> qint64 { return m_queue_id; }
auto DynamicIdQueue::configuration() const -> const QList<msg::model::QueueConfigField>& {
    return m_configuration;
}
auto DynamicIdQueue::configuring() const -> bool { return m_configuring; }
auto DynamicIdQueue::exhausted() const -> bool { return m_session.exhausted(); }
auto DynamicIdQueue::canRestart() const -> bool { return m_can_restart || m_session_expired; }
auto DynamicIdQueue::restartLabel() const -> QString {
    return m_session_expired ? tr("Restart radio") : m_restart_label;
}
auto DynamicIdQueue::description() const -> QString {
    const auto description = m_session.presentation().description();
    if (! m_session_expired) return description;
    const auto message = tr("Radio session expired. Restart to continue.");
    return description.isEmpty() ? message : description + QString::fromUtf8(" · ") + message;
}
auto DynamicIdQueue::values() const -> QHash<QString, QString> {
    QHash<QString, QString> values;
    for (const auto& field : m_configuration) values.insert(field.key(), field.value());
    return values;
}

void DynamicIdQueue::close(const QString& id) {
    if (id.isEmpty()) return;
    msg::CloseQueueReq req;
    req.setSessionId(id);
    msg::QcmMessage message;
    msg::MsgTraits<msg::CloseQueueReq>::set(message, std::move(req));
    App::instance()->backend()->send_immediate(std::move(message));
}

void DynamicIdQueue::invalidateSession() {
    restarting();
    static_cast<QueueNextQuery*>(m_query)->reset();
    static_cast<QueueNextQuery*>(m_open_query)->reset();
    close(m_session.id_proto());
    m_auto_removing = true;
    m_session = {};
    m_session_expired = false;
    m_configuring = false;
    m_suppress_prefetch = false;
    if (rowCount()) removeRows(0, rowCount());
    setCurrentIndex(-1);
    m_auto_removing = false;
    sessionChanged();
    configuringChanged();
}

void DynamicIdQueue::ensureStarted() {
    if (m_configuring) return;
    if (m_session_expired) {
        open(msg::QueueOpenReasonGadget::QueueOpenReason::QUEUE_OPEN_REASON_INITIAL, values());
        return;
    }
    if (!m_session.id_proto().isEmpty()) return;
    auto query = static_cast<QueueNextQuery*>(m_open_query);
    if (query->transportFailure()) {
        m_configuring = true;
        configuringChanged();
        query->reload();
        return;
    }
    open(msg::QueueOpenReasonGadget::QueueOpenReason::QUEUE_OPEN_REASON_INITIAL, values());
}

void DynamicIdQueue::open(msg::QueueOpenReasonGadget::QueueOpenReason reason,
                          const QHash<QString, QString>&              configuration) {
    if (m_configuring) return;
    msg::OpenQueueReq req;
    req.setQueueId(m_queue_id);
    req.setConfiguration(configuration);
    req.setReason(reason);
    req.setOperationId(QString::number(QDateTime::currentMSecsSinceEpoch()) + ':' +
                       QUuid::createUuid().toString(QUuid::WithoutBraces));
    if (! m_session_expired) req.setPreviousSessionId(m_session.id_proto());
    auto query = static_cast<QueueNextQuery*>(m_open_query);
    query->setRequest(req);
    m_configuring = true;
    configuringChanged();
    query->reload();
}

void DynamicIdQueue::restart() {
    if (canRestart())
        open(msg::QueueOpenReasonGadget::QueueOpenReason::QUEUE_OPEN_REASON_RESTART, values());
}

void DynamicIdQueue::configure(const QString& key, const QString& value) {
    if (m_configuring) return;
    const auto field = std::ranges::find(m_configuration, key, &msg::model::QueueConfigField::key);
    if (field == m_configuration.end() || field->value() == value) return;
    if (std::ranges::none_of(field->options(), [&value](const auto& option) {
            return option.value() == value;
        }))
        return;
    auto configuration = values();
    configuration.insert(key, value);
    open(msg::QueueOpenReasonGadget::QueueOpenReason::QUEUE_OPEN_REASON_RECONFIGURE, configuration);
}

void DynamicIdQueue::on_current_index_changed(qint32 idx) {
    if (m_auto_removing) return;
    if (idx > 1 && idx < rowCount()) {
        m_auto_removing = true;
        removeRows(0, idx - 1);
        m_auto_removing = false;
    }
    m_suppress_prefetch = false;
    on_request_next();
}

void DynamicIdQueue::on_request_next() {
    if (m_session.id_proto().isEmpty() || m_session_expired || m_session.exhausted() ||
        m_suppress_prefetch)
        return;
    auto query = static_cast<QueueNextQuery*>(m_query);
    if (query->querying() || currentIndex() < rowCount() - 3) return;
    request_next();
}

void DynamicIdQueue::request_next() {
    auto query = static_cast<QueueNextQuery*>(m_query);
    if (query->transportFailure()) {
        query->reload();
        return;
    }
    msg::GetQueueNextReq req;
    req.setQueueId(m_queue_id);
    QList<qint64> ids;
    for (int row = 0; row < rowCount(); ++row)
        ids.push_back(data(index(row), Qt::DisplayRole).value<model::ItemId>().id());
    req.setCurrentSongIds({ ids.begin(), ids.end() });
    req.setSessionId(m_session.id_proto());
    req.setExpectedVersion(m_session.version());
    req.setOperationId(QString::number(QDateTime::currentMSecsSinceEpoch()) + ':' +
                       QUuid::createUuid().toString(QUuid::WithoutBraces));
    query->setRequest(req);
    query->reload();
}

void DynamicIdQueue::on_open_finished() {
    auto query = static_cast<QueueNextQuery*>(m_open_query);
    if (query->querying()) return;
    m_configuring = false;
    if (query->status() != QAsyncResult::Status::Finished || query->tdata().songs().isEmpty() ||
        query->tdata().session().id_proto().isEmpty()) {
        if (query->status() == QAsyncResult::Status::Finished) {
            close(query->tdata().session().id_proto());
            Global::instance()->errorOccurred(tr("Radio returned no playable songs"));
        }
        if (query->sessionExpired()) {
            m_session_expired = true;
            sessionChanged();
        }
        configuringChanged();
        configurationChanged();
        return;
    }
    const auto& page     = query->tdata();
    const auto  previous = m_session.id_proto();
    restarting();
    static_cast<QueueNextQuery*>(m_query)->reset();
    m_auto_removing     = true;
    m_session           = page.session();
    m_session_expired   = false;
    m_suppress_prefetch = false;
    for (auto& field : m_configuration)
        field.setValue(m_session.configuration().value(field.key()));
    if (rowCount()) removeRows(0, rowCount());
    std::vector<model::ItemId> ids;
    for (const auto& song : page.songs())
        ids.emplace_back(enums::ItemType::ItemSong, song.id_proto());
    insert(0, ids);
    setCurrentIndex(0);
    m_auto_removing = false;
    sessionChanged();
    configurationChanged();
    configuringChanged();
    close(previous);
    restarted();
}

void DynamicIdQueue::on_query_finished() {
    auto query = static_cast<QueueNextQuery*>(m_query);
    if (query->querying()) return;
    if (query->status() != QAsyncResult::Status::Finished) {
        if (query->sessionExpired()) {
            m_session_expired = true;
            sessionChanged();
        }
        return;
    }
    const auto& page = query->tdata();
    if (page.session().id_proto() != m_session.id_proto() ||
        page.session().version() <= m_session.version())
        return;
    m_session                                 = page.session();
    const auto                 previous_count = rowCount();
    std::vector<model::ItemId> ids;
    for (const auto& song : page.songs())
        ids.emplace_back(enums::ItemType::ItemSong, song.id_proto());
    insert(rowCount(), ids);
    m_suppress_prefetch = rowCount() == previous_count;
    sessionChanged();
}

} // namespace qcm::model
