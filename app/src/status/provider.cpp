#include "Qcm/status/provider_status.hpp"

#include "Qcm/app.hpp"

namespace qcm
{
ProviderMetaStatusModel::ProviderMetaStatusModel(QObject* parent): Base(parent) {}
ProviderMetaStatusModel::~ProviderMetaStatusModel() {}
ProviderStatusModel::ProviderStatusModel(QObject* parent): Base(parent) {}
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

        auto idx = this->index(this->idx_at(id));
        dataChanged(idx, idx, { role });
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
} // namespace qcm

#include <Qcm/status/moc_provider_status.cpp>