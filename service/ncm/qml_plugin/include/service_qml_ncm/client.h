#pragma once
#include "qcm_interface/client.h"
#include "service_qml_ncm/export.h"

namespace ncm
{
class Client;
namespace qml
{
auto create_client() -> qcm::Client;
auto to_ncm_client(const qcm::Client& c) -> std::optional<ncm::Client>;
auto get_ncm_client() -> std::optional<ncm::Client>;

auto check(std::optional<ncm::Client>, const std::source_location = {}) -> bool;

SERVICE_QML_NCM_API auto uniq(const QUrl& url, const QVariant& info) -> QString;

constexpr std::string_view provider { "ncm" };

} // namespace qml
} // namespace ncm