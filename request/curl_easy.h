#pragma once

#include <functional>
#include <string_view>
#include <optional>

#include <tl/expected.hpp>
#include <curl/curl.h>
#include <asio/strand.hpp>
#include <asio/any_io_executor.hpp>

#include "core/core.h"
#include "curl_slist.h"
#include "type.h"

namespace request
{

constexpr CURLINFO to_curl_info(Attribute A) noexcept {
    switch (A) {
        using enum Attribute;
    case HttpCode: return CURLINFO_RESPONSE_CODE;
    default: return CURLINFO_RESPONSE_CODE;
    }
}

class CurlMulti;
class CurlEasy : NoCopy {
public:
    using done_callback_t = std::function<void(CURLcode)>;

    using executor_type = asio::strand<asio::any_io_executor>;

    CurlEasy(asio::any_io_executor) noexcept;
    ~CurlEasy();

    CURL* handle() const { return easy; }

    executor_type& get_strand() { return m_strand; }

    template<typename T>
    inline tl::expected<T, CURLcode> get_info(CURLINFO info) noexcept {
        T inst;
        if (auto res = curl_easy_getinfo(handle(), info, &inst)) {
            return tl::make_unexpected(res);
        }
        return inst;
    }

    template<typename T>
    CURLcode setopt(CURLoption opt, T para) noexcept {
        return curl_easy_setopt(handle(), opt, para);
    }
    CURLcode perform();

    void set_header(const Header&);
    void reset_header();

    CURLcode pause(int bitmask) noexcept { return curl_easy_pause(handle(), bitmask); }

    void done(CURLcode);
    void set_done_callback(done_callback_t);

private:
    CURL*           easy;
    curl_slist*     m_headers;
    done_callback_t m_done_cb;
    executor_type   m_strand;
};

} // namespace request
