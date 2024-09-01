#include "ncm/crypto.h"

#include <span>
#include <array>
#include <iostream>

#include "core/str_helper.h"
#include "core/random.h"
#include "core/log.h"
#include "request/type.h"

using namespace qcm;

constexpr std::array AES_IV  = "0102030405060708"_sb;
constexpr std::array AES_KEY = "0CoJUm6Qyw8W8jud"_sb;
constexpr std::array RSA_PUB_KEY =
    "-----BEGIN PUBLIC KEY-----\nMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDgtQn2JZ34ZC28NWYpAUd98iZ37BUrX/aKzmFbt7clFSs6sXqHauqKWqdtLkF2KexO40H1YTX8z2lSgBBOAxLsvaklV8k4cBFK9snQXE9/DDaFt6Rr7iVZMldczhC0JNgTz+SHXT6CBHuX3e9SdB1Ua44oncaTWz7OBGLbCiK45wIDAQAB\n-----END PUBLIC KEY-----"_sb;
constexpr std::array BASE62   = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"_sb;
constexpr std::array EAPI_KEY = "e82ckenh8dichen8"_sb;

ncm::Crypto::Crypto() {
    auto rsa_opt = crypto::rsa::Rsa::from_pem(RSA_PUB_KEY, {});
    m_rsa        = std::move(rsa_opt.value());
};
ncm::Crypto::~Crypto() {};

std::optional<std::string> ncm::Crypto::weapi(bytes_view in) {
    std::array<byte, 16> sec_key;
    for (auto& b : sec_key) {
        auto i = Random::get<usize>(0, 255) % 62;
        b      = BASE62[i];
    }

    auto params = crypto::encrypt(crypto::aes_128_cbc(), AES_KEY, AES_IV, in)
                      .and_then([](auto in) {
                          return crypto::encode(in);
                      })
                      .and_then([&sec_key](auto in) {
                          return crypto::encrypt(crypto::aes_128_cbc(), sec_key, AES_IV, in);
                      })
                      .and_then([](auto in) {
                          return crypto::encode(in);
                      })
                      .map(convert_from<std::string, bytes_view>);

    std::array<byte, 128> sec_key_padding { byte { 0 } };
    std::copy(sec_key.begin(), sec_key.end(), sec_key_padding.begin());
    std::reverse(sec_key_padding.begin(), sec_key_padding.end());

    auto enc_sec_key = m_rsa.encrypt(crypto::rsa::Padding::NONE, sec_key_padding)
                           .map(crypto::hex::encode_low)
                           .map(convert_from<std::string, bytes_view>);

    if (params.has_value() && enc_sec_key.has_value()) {
        request::UrlParams url_params;
        url_params.set_param("params", params.value()).set_param("encSecKey", enc_sec_key.value());

        return url_params.encode();
    }
    return std::nullopt;
}

std::optional<std::string> ncm::Crypto::eapi(std::string_view url, bytes_view in) {
    auto message = fmt::format("nobody{}use{}md5forencrypt", url, in);
    auto params  = crypto::digest(crypto::md5(), convert_from<std::vector<byte>>(message))
                      .map(crypto::hex::encode_low)
                      .map([&url, &in](auto msg) {
                          return convert_from<std::vector<byte>>(
                              fmt::format("{}-36cd479b6b5-{}-36cd479b6b5-{}", url, in, msg));
                      })
                      .and_then([](auto in) {
                          return crypto::encrypt(crypto::aes_128_ecb(), EAPI_KEY, AES_IV, in);
                      })
                      .map(crypto::hex::encode_up)
                      .map([](auto in) {
                          return fmt::format("params={}", in);
                      });
    if (params.has_value()) {
        return params.value();
    }
    return std::nullopt;
}
