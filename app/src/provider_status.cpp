module;
#include "Qcm/status/provider.moc.h"

module qcm;
import :status.provider;
import :app;

namespace qcm
{
ProviderMetaStatusModel::ProviderMetaStatusModel(QObject* parent)
    : kstore::QGadgetListModel(this, parent) {}
ProviderMetaStatusModel::~ProviderMetaStatusModel() {}
static constexpr std::string_view inactived_provider_key { "provider/inactived_providers" };
ProviderStatusModel::ProviderStatusModel(QObject* parent)
    : model::ItemIdListModel(this, parent, enums::ItemType::ItemProvider),
      m_syncing(false),
      m_lib_status(new LibraryStatus(this)) {
    QSettings s;
    for (const auto& v : s.value(inactived_provider_key).toStringList()) {
        m_inactived.insert(v.toLongLong());
    }

    connect(m_lib_status,
            &LibraryStatus::activedChanged,
            this,
            &ProviderStatusModel::libraryStatusChanged);
    connect(this, &ProviderStatusModel::activedChanged, this, &ProviderStatusModel::activedIdsChanged);
    connect(this, &ProviderStatusModel::activedChanged, this, [this](i64, bool) {
        QSettings   s;
        QStringList list;
        for (auto el : m_inactived) list.emplaceBack(QString::number(el));
        s.setValue(inactived_provider_key, list);
        m_lib_status->activedIdsChanged();
    });
}
ProviderStatusModel::~ProviderStatusModel() {}

void ProviderStatusModel::updateSyncStatus(const msg::model::ProviderSyncStatus& s) {
    static auto role = Qt::UserRole + 1 +
                       msg::model::ProviderStatus::staticMetaObject.indexOfProperty("syncStatus");
    auto        id   = s.id_proto();
    if (auto v = this->query(id); v) {
        auto& value = *v;
        value.setSyncStatus(s);

        if (s.state() == msg::model::SyncStateGadget::SyncState::SYNC_STATE_SYNCING) {
            setSyncing(true);
        } else {
            checkSyncing();
        }

        if (auto idx = this->query_idx(id); idx) {
            auto qidx = this->index(*idx);
            dataChanged(qidx, qidx, { role });
        }
    }
}

auto ProviderStatusModel::metaById(const model::ItemId& item_id) const -> QVariant {
    auto p = this->query(item_id.id());
    if (p) {
        auto metas = App::instance()->provider_meta_status();
        if (auto m = metas->query(p->typeName())) return QVariant::fromValue(*m);
    }
    return {};
}

auto ProviderStatusModel::svg(qint32 idx) const -> QString {
    if (this->rowCount() > idx && idx >= 0) {
        auto& p     = this->at(idx);
        auto  metas = App::instance()->provider_meta_status();
        if (auto m = metas->query(p.typeName())) return m->svg();
    }
    return "";
}

auto ProviderStatusModel::svg(const model::ItemId& item_id) const -> QString {
    auto p = this->query(item_id.id());
    if (p) {
        auto metas = App::instance()->provider_meta_status();
        if (auto m = metas->query(p->typeName())) return m->svg();
    }
    return "";
}

QVariant ProviderStatusModel::itemById(const model::ItemId& item_id) const {
    auto p = this->query(item_id.id());
    if (p) return QVariant::fromValue(*p);
    return {};
}

auto ProviderStatusModel::syncing() const -> bool { return m_syncing; }

void ProviderStatusModel::setSyncing(bool value) {
    if (m_syncing != value) {
        m_syncing = value;
        syncingChanged(value);
    }
}

void ProviderStatusModel::checkSyncing() {
    bool syncing = false;
    for (auto i = 0; i < rowCount(); ++i) {
        auto& provider = this->at(i);
        if (provider.syncStatus().state() ==
            msg::model::SyncStateGadget::SyncState::SYNC_STATE_SYNCING) {
            syncing = true;
            break;
        }
    }
    setSyncing(syncing);
}

auto ProviderStatusModel::libraryStatus() const -> LibraryStatus* { return m_lib_status; }

auto ProviderStatusModel::activedIds() -> const QtProtobuf::int64List& {
    m_actived_ids.clear();
    for (auto i = 0; i < rowCount(); ++i) {
        auto id = this->at(i).id_proto();
        if (actived(id)) m_actived_ids.emplaceBack(id);
    }
    return m_actived_ids;
}

bool ProviderStatusModel::actived(i64 id) const { return ! m_inactived.contains(id); }

void ProviderStatusModel::setActived(i64 id, bool value) {
    bool active = ! m_inactived.contains(id);
    if (active == value) return;
    if (active) {
        m_inactived.insert(id);
    } else {
        m_inactived.erase(id);
    }
    activedChanged(id, value);
}

static constexpr std::string_view inactived_library_key { "provider/inactived_libraries" };

LibraryStatus::LibraryStatus(QObject* parent): QObject(parent) {
    QSettings s;
    for (const auto& v : s.value(inactived_library_key).toStringList()) {
        m_inactived.insert(v.toLongLong());
    }

    connect(this, &LibraryStatus::activedChanged, this, &LibraryStatus::activedIdsChanged);
    connect(this, &LibraryStatus::activedChanged, this, [this](i64, bool) {
        QSettings   s;
        QStringList list;
        for (auto el : m_inactived) list.emplaceBack(QString::number(el));
        s.setValue(inactived_library_key, list);
    });
}

LibraryStatus::~LibraryStatus() {}

auto LibraryStatus::activedIds() -> const QtProtobuf::int64List& {
    m_ids.clear();
    auto providers = App::instance()->provider_status();
    for (auto i = 0; i < providers->rowCount(); ++i) {
        auto& provider = providers->at(i);
        if (! providers->actived(provider.id_proto())) continue;
        for (auto& library : provider.libraries()) {
            auto id = library.libraryId();
            if (actived(id)) m_ids.emplaceBack(id);
        }
    }
    return m_ids;
}

bool LibraryStatus::actived(i64 id) const { return ! m_inactived.contains(id); }

void LibraryStatus::setActived(i64 id, bool value) {
    bool active = ! m_inactived.contains(id);
    if (active == value) return;
    if (active) {
        m_inactived.insert(id);
    } else {
        m_inactived.erase(id);
    }
    activedChanged(id, value);
}
} // namespace qcm

#include "Qcm/status/provider.moc.cpp"
