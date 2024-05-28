#pragma once

#include <functional>
#include <string_view>
#include <optional>

#include <tl/expected.hpp>
#include <curl/curl.h>

#include "core/core.h"
#include "core/fmt.h"
#include "request/type.h"

namespace request
{
constexpr CURLINFO to_curl_info(Attribute A) noexcept {
    switch (A) {
        using enum Attribute;
    case HttpCode: return CURLINFO_RESPONSE_CODE;
    default: return CURLINFO_RESPONSE_CODE;
    }
}

class CurlEasy : NoCopy {
public:
    CurlEasy() noexcept: easy(curl_easy_init()), m_headers(NULL) {
        // enable cookie engine
        setopt(CURLOPT_COOKIEFILE, "");

        // thread safe
        setopt(CURLOPT_NOSIGNAL, 1L);

        setopt(CURLOPT_FOLLOWLOCATION, 1L);
        setopt(CURLOPT_AUTOREFERER, 1L);
        setopt(CURLOPT_VERBOSE, 0L);
    }

    ~CurlEasy() {
        reset_header();
        curl_easy_cleanup(easy);
    }

    CURL* handle() const { return easy; }

    template<typename T>
    auto curl_private() {
        return get_info<T>(CURLINFO_PRIVATE);
    }

    template<typename T>
    inline nstd::expected<T, CURLcode> get_info(CURLINFO info) noexcept {
        T inst;
        if (auto res = curl_easy_getinfo(handle(), info, &inst)) {
            return nstd::unexpected(res);
        }
        return inst;
    }

    template<typename T>
    CURLcode setopt(CURLoption opt, T para) noexcept {
        return curl_easy_setopt(handle(), opt, para);
    }
    CURLcode perform() { return curl_easy_perform(easy); }

    void set_header(const Header& headers) {
        reset_header();
        for (auto& [k, v] : headers) {
            std::string header = fmt::format("{}: {}", k, v);
            m_headers          = curl_slist_append(m_headers, header.c_str());
        }
        if (m_headers != NULL) setopt(CURLOPT_HTTPHEADER, m_headers);
    }

    void reset_header() {
        setopt(CURLOPT_HTTPHEADER, NULL);
        curl_slist_free_all(m_headers);
        m_headers = NULL;
    }

    CURLcode pause(int bitmask) noexcept { return curl_easy_pause(handle(), bitmask); }

private:
    CURL*       easy;
    curl_slist* m_headers;
};
} // namespace request
