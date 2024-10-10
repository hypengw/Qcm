#pragma once
#include "qcm_interface/client.h"
#include "service_qml_ncm/export.h"

namespace ncm
{
class Client;
namespace qml
{
auto                     create_client() -> qcm::Client;
auto                     get_ncm_client(qcm::Client& c) -> ncm::Client&;
SERVICE_QML_NCM_API auto uniq(const QUrl& url, const QVariant& info) -> QString;

constexpr std::string_view provider { "ncm" };

} // namespace qml
} // namespace ncm