#include "crypto.h"

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include <openssl/rsa.h>
#include <openssl/engine.h>
#include <openssl/decoder.h>

#include <string_view>

#include "core/expected_helper.h"
#include "core/str_helper.h"

using namespace qcm;

using evp_cipher_ctx   = std::unique_ptr<EVP_CIPHER_CTX, decltype(&::EVP_CIPHER_CTX_free)>;
using evp_encode_ctx   = std::unique_ptr<EVP_ENCODE_CTX, decltype(&::EVP_ENCODE_CTX_free)>;
using evp_pkey_ctx     = std::unique_ptr<EVP_PKEY_CTX, decltype(&::EVP_PKEY_CTX_free)>;
using evp_md_ctx       = std::unique_ptr<EVP_MD_CTX, decltype(&::EVP_MD_CTX_free)>;
using ossl_decoder_ctx = std::unique_ptr<OSSL_DECODER_CTX, decltype(&::OSSL_DECODER_CTX_free)>;

const EVP_CIPHER* crypto::aes_128_ecb() noexcept { return EVP_aes_128_ecb(); }
const EVP_CIPHER* crypto::aes_128_cbc() noexcept { return EVP_aes_128_cbc(); }
const EVP_MD*     crypto::md5() noexcept { return EVP_md5(); }

namespace
{
int to_padding(crypto::rsa::Padding pad) {
    switch (pad) {
        using enum crypto::rsa::Padding;
    case NONE: return RSA_NO_PADDING;
    case PKCS1: return RSA_PKCS1_PADDING;
    case PKCS1_OAEP: return RSA_PKCS1_OAEP_PADDING;
    case X931: return RSA_X931_PADDING;
    default: return RSA_NO_PADDING;
    }
}
} // namespace
nstd::expected<std::vector<byte>, int> crypto::encrypt(const EVP_CIPHER* cipher, bytes_view key,
                                                       bytes_view iv, bytes_view data) {
    evp_cipher_ctx ctx_ { EVP_CIPHER_CTX_new(), ::EVP_CIPHER_CTX_free };
    auto           ctx = ctx_.get();

    int rc = EVP_EncryptInit_ex(
        ctx, cipher, NULL, (unsigned char*)key.data(), (unsigned char*)iv.data());
    if (rc != 1) return nstd::unexpected(0);

    std::vector<byte> out(data.size() + iv.size());

    int out_len1 = (int)out.size();

    rc = EVP_EncryptUpdate(
        ctx, (unsigned char*)out.data(), &out_len1, (unsigned char*)data.data(), (int)data.size());

    if (rc != 1) return nstd::unexpected(0);

    int out_len2 = (int)out.size() - out_len1;

    rc = EVP_EncryptFinal_ex(ctx, (unsigned char*)out.data() + out_len1, &out_len2);
    if (rc != 1) return nstd::unexpected(0);

    // Set cipher text size now that we know it
    out.resize(out_len1 + out_len2);
    return out;
}
nstd::expected<std::vector<byte>, int> crypto::decrypt(const EVP_CIPHER* cipher, bytes_view key,
                                                       bytes_view iv, bytes_view in) {
    evp_cipher_ctx ctx_ { EVP_CIPHER_CTX_new(), ::EVP_CIPHER_CTX_free };
    auto           ctx = ctx_.get();

    int rc = EVP_DecryptInit_ex(
        ctx, cipher, NULL, (unsigned char*)key.data(), (unsigned char*)iv.data());
    if (rc != 1) return nstd::unexpected(0);

    std::vector<byte> out(in.size());

    int out_len1 = (int)out.size();

    rc = EVP_DecryptUpdate(
        ctx, (unsigned char*)out.data(), &out_len1, (unsigned char*)in.data(), (int)in.size());
    if (rc != 1) return nstd::unexpected(0);

    int out_len2 = (int)out.size() - out_len1;
    rc           = EVP_DecryptFinal_ex(ctx, (unsigned char*)out.data() + out_len1, &out_len2);

    if (rc != 1) return nstd::unexpected(0);

    // Set recovered text size now that we know it
    out.resize(out_len1 + out_len2);
    return out;
}

nstd::expected<std::vector<byte>, int> crypto::encode(bytes_view in) {
    evp_encode_ctx ctx_ { EVP_ENCODE_CTX_new(), ::EVP_ENCODE_CTX_free };
    auto           ctx = ctx_.get();
    EVP_EncodeInit(ctx);

    int               max_out_size = 1 + 65 * ((int)(in.size() / 48) + 1);
    std::vector<byte> out(max_out_size);

    int  total { 0 }, out_size { 0 }, in_size { (int)in.size() };
    auto out_data = (unsigned char*)out.data();
    auto in_data  = (unsigned char*)in.data();

    if (EVP_EncodeUpdate(ctx, out_data, &out_size, in_data, in_size) != 1)
        return nstd::unexpected(1);
    total += out_size;

    EVP_EncodeFinal(ctx, out_data + total, &out_size);

    out.resize(total + out_size);
    return out;
}

nstd::expected<std::vector<byte>, int> crypto::decode(bytes_view in) {
    evp_encode_ctx ctx_ { EVP_ENCODE_CTX_new(), ::EVP_ENCODE_CTX_free };
    auto           ctx = ctx_.get();
    EVP_DecodeInit(ctx);

    int               max_out_size = 1 + 48 * ((int)(in.size() / 64) + 1);
    std::vector<byte> out(max_out_size);

    int  total { 0 }, out_size { 0 }, in_size { (int)in.size() };
    auto out_data = (unsigned char*)out.data();
    auto in_data  = (unsigned char*)in.data();

    int rc = EVP_DecodeUpdate(ctx, out_data, &out_size, in_data, in_size);
    if (rc != 1 && rc != 0) return nstd::unexpected(1);
    total += out_size;

    if (EVP_DecodeFinal(ctx, out_data + total, &out_size) != 1) return nstd::unexpected(1);

    out.resize(total + out_size);
    return out;
}

nstd::expected<std::vector<byte>, int> crypto::digest(const md* type, bytes_view data) {
    evp_md_ctx        ctx_ { EVP_MD_CTX_new(), ::EVP_MD_CTX_free };
    auto              ctx = ctx_.get();
    std::vector<byte> out(EVP_MAX_MD_SIZE);
    unsigned int      out_size = out.size();

    if (EVP_DigestInit(ctx, type) != 1) return nstd::unexpected(1);
    if (EVP_DigestUpdate(ctx, data.data(), data.size()) != 1) return nstd::unexpected(1);
    if (EVP_DigestFinal(ctx, (unsigned char*)out.data(), &out_size) != 1)
        return nstd::unexpected(1);

    out.resize(out_size);
    return out;
}

auto crypto::digest(const md* type, usize buf_size,
                    const reader& reader) -> nstd::expected<std::vector<byte>, int> {
    evp_md_ctx        ctx_ { EVP_MD_CTX_new(), ::EVP_MD_CTX_free };
    auto              ctx = ctx_.get();
    std::vector<byte> out(EVP_MAX_MD_SIZE);
    unsigned int      out_size = out.size();

    std::vector<byte> buf(buf_size);

    if (EVP_DigestInit(ctx, type) != 1) return nstd::unexpected(1);
    for (;;) {
        auto size = reader(buf);
        if (size == 0) break;
        if (EVP_DigestUpdate(ctx, buf.data(), size) != 1) return nstd::unexpected(1);
    }
    if (EVP_DigestFinal(ctx, (unsigned char*)out.data(), &out_size) != 1)
        return nstd::unexpected(1);

    out.resize(out_size);
    return out;
}

std::vector<byte> crypto::hex::encode_low(bytes_view data) {
    constexpr std::array hex_digits = "0123456789abcdef"_sb;

    std::vector<byte> out;
    for (auto& b : data) {
        auto i = std::to_integer<usize>(b >> 4);
        out.push_back(hex_digits[i]);
        i = std::to_integer<usize>((b << 4) >> 4);
        out.push_back(hex_digits[i]);
    }
    return out;
}

std::vector<byte> crypto::hex::encode_up(bytes_view data) {
    constexpr std::array hex_digits = "0123456789ABCDEF"_sb;

    std::vector<byte> out;
    for (auto& b : data) {
        auto i = std::to_integer<usize>(b >> 4);
        out.push_back(hex_digits[i]);
        i = std::to_integer<usize>((b << 4) >> 4);
        out.push_back(hex_digits[i]);
    }
    return out;
}

namespace qcm::crypto
{
struct Pkey {
    Pkey(): pkey(NULL) {}
    ~Pkey() { reset(); }

    operator bool() const noexcept { return pkey != NULL; }

    void reset() { EVP_PKEY_free(pkey); }

    EVP_PKEY* pkey;
};

rsa::Rsa::Rsa(): key(std::make_unique<Pkey>()) {}
rsa::Rsa::~Rsa() {}

rsa::Rsa::Rsa(Rsa&& o) noexcept: key(std::exchange(o.key, nullptr)) {}
rsa::Rsa& rsa::Rsa::operator=(Rsa&& o) noexcept {
    key = std::exchange(o.key, nullptr);
    return *this;
}

std::optional<rsa::Rsa> rsa::Rsa::from_pem(bytes_view data, bytes_view pass) {
    Rsa rsa;

    auto dctx_ = ossl_decoder_ctx(
        OSSL_DECODER_CTX_new_for_pkey(
            &(rsa.key->pkey), "PEM", NULL, "RSA", OSSL_KEYMGMT_SELECT_PUBLIC_KEY, NULL, NULL),
        ::OSSL_DECODER_CTX_free);

    if (! dctx_) {
        return std::nullopt;
    }
    auto dctx = dctx_.get();
    if (! pass.empty())
        OSSL_DECODER_CTX_set_passphrase(dctx, (unsigned char*)pass.data(), (int)pass.size());

    auto data_ptr = (const unsigned char*)data.data();
    auto data_len = (size_t)data.size();

    if (OSSL_DECODER_from_data(dctx, &data_ptr, &data_len)) {
        return rsa;
    }
    return std::nullopt;
}

nstd::expected<std::vector<byte>, int> rsa::Rsa::encrypt(Padding pad, bytes_view data) {
    if (auto ctx_ =
            evp_pkey_ctx(EVP_PKEY_CTX_new_from_pkey(NULL, key->pkey, NULL), ::EVP_PKEY_CTX_free);
        ctx_) {
        auto ctx = ctx_.get();

        std::vector<byte> out;
        size_t            out_len { out.size() };

        auto in     = (unsigned char*)data.data();
        auto in_len = (size_t)data.size();

        if (EVP_PKEY_encrypt_init(ctx) == 1)
            if (EVP_PKEY_CTX_set_rsa_padding(ctx, to_padding(pad)) == 1)
                if (EVP_PKEY_encrypt(ctx, NULL, &out_len, in, in_len) == 1) {
                    out.resize(out_len);
                    if (EVP_PKEY_encrypt(ctx, (unsigned char*)out.data(), &out_len, in, in_len) ==
                        1) {
                        return out;
                    }
                }
    }
    return nstd::unexpected(1);
}

} // namespace qcm::crypto
