#include "Qcm/status/provider_status.hpp"

#include "Qcm/app.hpp"
#include <QtCore/QSettings>

namespace qcm
{
ProviderMetaStatusModel::ProviderMetaStatusModel(QObject* parent): Base(parent) {}
ProviderMetaStatusModel::~ProviderMetaStatusModel() {}
ProviderStatusModel::ProviderStatusModel(QObject* parent)
    : Base(parent), m_lib_status(new LibraryStatus(this)) {
    connect(m_lib_status,
            &LibraryStatus::activedChanged,
            this,
            &ProviderStatusModel::libraryStatusChanged);
}
ProviderStatusModel::~ProviderStatusModel() {}

void ProviderStatusModel::updateSyncStatus(const msg::model::ProviderSyncStatus& s) {
    static auto role = Qt::UserRole + 1 +
                       msg::model::ProviderStatus::staticMetaObject.indexOfProperty("syncStatus");
    auto id = s.id_proto();
    if (auto v = this->query(id); v) {
        auto& value = *v;
        value.setSyncStatus(s);

        if (s.state() == msg::model::SyncStateGadget::SyncState::Syncing) {
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

auto ProviderStatusModel::svg(qint32 idx) const -> QString {
    auto& p     = this->at(idx);
    auto  metas = App::instance()->provider_meta_status();
    if (auto m = metas->query(p.typeName())) {
        return m->svg();
    }
    return "";
}
auto ProviderStatusModel::syncing() const -> bool { return m_syncing; }

void ProviderStatusModel::setSyncing(bool v) {
    if (m_syncing != v) {
        m_syncing = v;
        syncingChanged(v);
    }
}
void ProviderStatusModel::checkSyncing() {
    bool out = false;
    for (auto i = 0; i < rowCount(); i++) {
        auto& p = this->at(i);
        if (p.syncStatus().state() == msg::model::SyncStateGadget::SyncState::Syncing) {
            out = true;
            break;
        }
    }
    setSyncing(out);
}

auto ProviderStatusModel::libraryStatus() const -> LibraryStatus* { return m_lib_status; }

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
    auto p = App::instance()->provider_status();
    for (auto i = 0; i < p->rowCount(); i++) {
        auto& provider = p->at(i);
        for (auto& l : provider.libraries()) {
            auto id = l.libraryId();
            if (actived(id)) m_ids.emplaceBack(id);
        }
    }
    return m_ids;
}

bool LibraryStatus::actived(i64 id) const { return ! m_inactived.contains(id); }
void LibraryStatus::setActived(i64 id, bool v) {
    bool has = ! m_inactived.contains(id);
    if (has != v) {
        if (has) {
            m_inactived.insert(id);
        } else {
            m_inactived.erase(id);
        }
        activedChanged(id, v);
    }
}
} // namespace qcm

#include <Qcm/status/moc_provider_status.cpp>