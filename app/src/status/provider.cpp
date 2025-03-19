#include "Qcm/status/provider_status.hpp"

namespace qcm
{
ProviderStatusModel::ProviderStatusModel(QObject* parent): Base(parent) {}
ProviderStatusModel::~ProviderStatusModel() {}
} // namespace qcm

#include <Qcm/status/moc_provider_status.cpp>