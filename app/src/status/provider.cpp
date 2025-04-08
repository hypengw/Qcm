#include "Qcm/status/provider_status.hpp"

#include "Qcm/app.hpp"

namespace qcm
{
ProviderMetaStatusModel::ProviderMetaStatusModel(QObject* parent): Base(parent) {}
ProviderMetaStatusModel::~ProviderMetaStatusModel() {}
ProviderStatusModel::ProviderStatusModel(QObject* parent): Base(parent) {}
ProviderStatusModel::~ProviderStatusModel() {}

auto ProviderStatusModel::svg(qint32 idx) const -> QString {
    auto& p     = this->at(idx);
    auto metas = App::instance()->provider_meta_status();
    if (auto m = metas->query(p.typeName())) {
        return m->svg();
    }
    return "";
}
} // namespace qcm

#include <Qcm/status/moc_provider_status.cpp>