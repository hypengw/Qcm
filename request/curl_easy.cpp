#include "curl_easy.h"
#include "curl_multi.h"
#include <fmt/core.h>

using namespace request;

CurlEasy::CurlEasy(asio::any_io_executor ex) noexcept
    : easy(curl_easy_init()), m_headers(NULL), m_strand(ex) {
    // enable cookie engine
    setopt(CURLOPT_COOKIEFILE, "");

    // thread safe
    setopt(CURLOPT_NOSIGNAL, 1L);

    setopt(CURLOPT_FOLLOWLOCATION, 1L);
    setopt(CURLOPT_AUTOREFERER, 1L);
    setopt(CURLOPT_VERBOSE, 0L);
    setopt(CURLOPT_PRIVATE, this);
}

CurlEasy::~CurlEasy() {
    reset_header();
    curl_easy_cleanup(easy);
}

CURLcode CurlEasy::perform() { return curl_easy_perform(easy); }

void CurlEasy::set_header(const Header& headers) {
    reset_header();
    for (auto& [k, v] : headers) {
        std::string header = fmt::format("{}: {}", k, v);
        m_headers          = curl_slist_append(m_headers, header.c_str());
    }
    if (m_headers != NULL) setopt(CURLOPT_HTTPHEADER, m_headers);
}

void CurlEasy::reset_header() {
    setopt(CURLOPT_HTTPHEADER, NULL);
    curl_slist_free_all(m_headers);
    m_headers = NULL;
}
/*
void CurlEasy::add_header(std::string_view name, std::optional<std::string_view> val) {
    std::string head;
    if (val.has_value()) {
        head = fmt::format("{}: {}", name, val.value());
    } else {
        head = fmt::format("{};", name);
    }
    m_headers.push_back(CurlSlist::Item(head));
    setopt(CURLOPT_HTTPHEADER, m_headers.handle());
}

void CurlEasy::remove_header(std::string_view name) {
    m_headers.push_back(CurlSlist::Item(name));
    setopt(CURLOPT_HTTPHEADER, m_headers.handle());
}
*/

void CurlEasy::done(CURLcode cc) {
    if (m_done_cb) {
        m_done_cb(cc);
    }
}

void CurlEasy::set_done_callback(done_callback_t cb) { m_done_cb = std::move(cb); }
