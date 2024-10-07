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

namespace detail
{
template<CURLoption OPT>
struct curl_opt_traits;
template<>
struct curl_opt_traits<CURLoption::CURLOPT_SHARE> {
    using type = CURLSH*;
};
} // namespace detail

class CurlEasy : NoCopy {
public:
    CurlEasy() noexcept: easy(curl_easy_init()), m_headers(NULL), m_share(NULL) {
        // enable cookie engine
        setopt<CURLOPT_COOKIEFILE>("");

        // thread safe
        setopt<CURLOPT_NOSIGNAL>(1L);

        setopt<CURLOPT_FOLLOWLOCATION>(1L);
        setopt<CURLOPT_AUTOREFERER>(1L);
        setopt<CURLOPT_VERBOSE>(0L);
    }

    ~CurlEasy() {
        reset_header();
        curl_easy_cleanup(easy);
    }

    CURL* handle() const noexcept { return easy; }

    template<typename T>
    auto curl_private() {
        return get_info<T>(CURLINFO_PRIVATE);
    }

    template<typename T>
    inline auto get_info(CURLINFO info) noexcept -> nstd::expected<T, CURLcode> {
        T inst;
        if (auto res = curl_easy_getinfo(handle(), info, &inst)) {
            return nstd::unexpected(res);
        }
        return inst;
    }

    template<CURLoption OPT>
    auto getopt() noexcept -> typename detail::curl_opt_traits<OPT>::type {
        static_assert(false);
    }

    template<CURLoption OPT, typename T>
    constexpr auto setopt(T para) noexcept -> CURLcode {
        return curl_easy_setopt(handle(), OPT, para);
    }

    template<typename T>
    auto setopt(CURLoption opt, T para) noexcept -> CURLcode {
        return curl_easy_setopt(handle(), opt, para);
    }

    CURLcode perform() noexcept { return curl_easy_perform(easy); }

    void set_header(const Header& headers) {
        reset_header();
        for (auto& [k, v] : headers) {
            std::string header = fmt::format("{}: {}", k, v);
            m_headers          = curl_slist_append(m_headers, header.c_str());
        }
        if (m_headers != NULL) setopt<CURLOPT_HTTPHEADER>(m_headers);
    }

    void reset_header() {
        setopt<CURLOPT_HTTPHEADER>(nullptr);
        curl_slist_free_all(m_headers);
        m_headers = NULL;
    }

    CURLcode pause(int bitmask) noexcept { return curl_easy_pause(handle(), bitmask); }

private:
    CURL*       easy;
    curl_slist* m_headers;
    CURLSH*     m_share;
};

template<>
inline auto
CurlEasy::setopt<CURLoption::CURLOPT_SHARE, CURLSH*>(CURLSH* para) noexcept -> CURLcode {
    auto code = curl_easy_setopt(handle(), CURLoption::CURLOPT_SHARE, para);
    m_share   = para;
    return code;
}

template<>
inline auto CurlEasy::getopt<CURLoption::CURLOPT_SHARE>() noexcept -> CURLSH* {
    return m_share;
}
} // namespace request
