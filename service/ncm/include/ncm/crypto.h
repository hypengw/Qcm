#pragma once
#include "core/core.h"
#include "crypto/crypto.h"

#include <optional>

namespace ncm
{

using Rsa        = qcm::crypto::rsa::Rsa;
using bytes_view = qcm::crypto::bytes_view;

class Crypto : NoCopy {
public:
    Crypto();
    ~Crypto();

    auto weapi(bytes_view) -> std::optional<std::string>;
    auto eapi(std::string_view url, bytes_view) -> std::optional<std::string>;

private:
    Rsa m_rsa;
};

} // namespace ncm
