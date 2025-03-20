#include "Qcm/status/provider_status.hpp"

namespace qcm
{
ProviderMetaStatusModel::ProviderMetaStatusModel(QObject* parent): Base(parent) {}
ProviderMetaStatusModel::~ProviderMetaStatusModel() {}
ProviderStatusModel::ProviderStatusModel(QObject* parent): Base(parent) {}
ProviderStatusModel::~ProviderStatusModel() {}
} // namespace qcm

#include <Qcm/status/moc_provider_status.cpp>