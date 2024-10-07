#pragma once
#include "qcm_interface/client.h"

namespace ncm
{
class Client;
namespace qml
{
auto create_client() -> qcm::Client;
auto get_ncm_client(qcm::Client& c) -> ncm::Client&;

constexpr std::string_view provider { "ncm" };

} // namespace qml
} // namespace ncm