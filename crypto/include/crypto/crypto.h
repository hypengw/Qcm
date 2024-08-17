#include "core/core.h"

#include <optional>
#include <span>
#include <functional>

#include "core/expected_helper.h"

struct evp_cipher_st;
struct evp_md_st;

namespace qcm
{

namespace crypto
{
using reader     = std::function<usize(std::span<byte>)>;
using bytes_view = std::span<const byte>;
using cipher     = evp_cipher_st;
using md         = evp_md_st;

const cipher* aes_128_ecb() noexcept;
const cipher* aes_128_cbc() noexcept;
const md*     md5() noexcept;

auto encrypt(const cipher* cipher, bytes_view key, bytes_view iv,
             bytes_view data) -> nstd::expected<std::vector<byte>, int>;
auto decrypt(const cipher* cipher, bytes_view key, bytes_view iv,
             bytes_view data) -> nstd::expected<std::vector<byte>, int>;
auto encode(bytes_view data) -> nstd::expected<std::vector<byte>, int>;
auto decode(bytes_view data) -> nstd::expected<std::vector<byte>, int>;
auto digest(const md* type, bytes_view data) -> nstd::expected<std::vector<byte>, int>;
auto digest(const md* type, usize buf_size, const reader&) -> nstd::expected<std::vector<byte>, int>;

namespace hex
{
std::vector<byte> encode_low(bytes_view data);
std::vector<byte> encode_up(bytes_view data);
} // namespace hex

struct Pkey;

namespace rsa
{
enum class Padding
{
    NONE,
    PKCS1,
    PKCS1_OAEP,
    X931,
};

class Rsa : NoCopy {
public:
    Rsa();
    ~Rsa();

    Rsa(Rsa&&) noexcept;
    Rsa& operator=(Rsa&&) noexcept;

    static auto from_pem(bytes_view data, bytes_view pass) -> std::optional<Rsa>;
    auto        encrypt(Padding, bytes_view data) -> nstd::expected<std::vector<byte>, int>;

private:
    up<Pkey> key;
};

} // namespace rsa
} // namespace crypto
} // namespace qcm
