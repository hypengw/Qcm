#pragma once
#include "core/core.h"
#include "crypto/crypto.h"

#include <optional>
#include <vector>
#include <span>

namespace ncm
{

using Rsa = qcm::crypto::rsa::Rsa;
using bytes_view = qcm::crypto::bytes_view;

class Crypto : NoCopy {
public:
    Crypto();
    ~Crypto();

    std::optional<std::string> weapi(bytes_view);
    std::optional<std::string> eapi(std::string_view url, bytes_view);

private:
    Rsa m_rsa;
};

} // namespace ncm
