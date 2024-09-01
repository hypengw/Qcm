#include "type.h"

#include <curl/curl.h>
#include <fmt/core.h>

#include "core/str_helper.h"

using namespace request;

std::string request::url_encode(std::string_view c) {
    char*       curl_out = curl_easy_escape(NULL, c.data(), c.size());
    std::string out      = curl_out;
    curl_free(curl_out);
    return out;
}
std::string request::url_decode(std::string_view c) {
    int         len { 0 };
    char*       curl_out = curl_easy_unescape(NULL, c.data(), c.size(), &len);
    usize       slen     = len > 0 ? (usize)len : 0u;
    std::string out { curl_out, slen };
    curl_free(curl_out);
    return out;
}

bool CaseInsensitiveCompare::operator()(std::string_view a, std::string_view b) const noexcept {
    return helper::case_insensitive_compare(a, b) < 0;
}

std::string_view UrlParams::param(std::string_view name) const {
    if (auto it = m_params.find(name); it != m_params.end()) {
        return it->second.front();
    }
    return {};
}

UrlParams& UrlParams::set_param(std::string_view name, std::string_view val) {
    m_params.insert_or_assign(std::string(name), std::vector { std::string(val) });
    return *this;
}

auto UrlParams::params(std::string_view name) const -> std::vector<std::string_view> {
    if (auto it = m_params.find(name); it != m_params.end()) {
        return { it->second.begin(), it->second.end() };
    }
    return {};
}
auto UrlParams::is_array(std::string_view name) const -> bool {
    if (auto it = m_params.find(name); it != m_params.end()) {
        return it->second.size() > 1;
    }
    return false;
}
auto UrlParams::add_param(std::string_view name, std::string_view val) -> UrlParams& {
    if (auto it = m_params.find(name); it != m_params.end()) {
        it->second.push_back(std::string(val));
    } else {
        set_param(name, val);
    }
    return *this;
}

void UrlParams::decode(std::string_view) {}

std::string UrlParams::encode() const {
    std::string res;
    bool        first = true;
    for (auto& [k, v] : m_params) {
        if (! first)
            res.push_back('&');
        else
            first = false;

        if (v.size() > 1) {
            usize i = 0;
            for (auto& el : v) {
                res.append(fmt::format("{}[{}]={}", url_encode(k), i, url_encode(el)));
            }
        } else {
            res.append(fmt::format("{}={}", url_encode(k), url_encode(v.front())));
        }
    }
    return res;
}

/*
Url Url::from(std::string_view url_) {
    auto curlu_ =
        std::unique_ptr<CURLU, decltype(&::curl_url_cleanup)>(curl_url(), ::curl_url_cleanup);
    auto h = curlu_.get();

    Url o;

    auto flag = CURLU_URLDECODE;
    o.url     = url_;

    auto get_part = [&h, flag](CURLUPart p, std::string& o) {
        char* part;
        auto  rc = curl_url_get(h, p, &part, flag);
        if (rc == CURLUcode::CURLUE_OK) {
            o = part;
        }
        curl_free(part);
    };
    auto rc = curl_url_set(h, CURLUPART_URL, o.url.c_str(), 0);

    if (rc == CURLUcode::CURLUE_OK) {
        get_part(CURLUPART_HOST, o.host);
        get_part(CURLUPART_HOST, o.host);
        get_part(CURLUPART_SCHEME, o.scheme);
        get_part(CURLUPART_USER, o.user);
        get_part(CURLUPART_PASSWORD, o.password);
        get_part(CURLUPART_PORT, o.port);
        get_part(CURLUPART_PATH, o.path);
        get_part(CURLUPART_QUERY, o.query);
        get_part(CURLUPART_FRAGMENT, o.fragment);
    }

    return o;
}
*/