#pragma once

#include <unordered_map>
#include <filesystem>
#include <mutex>
#include <set>

#include <curl/curl.h>
#include <asio/steady_timer.hpp>

#include "curl_easy.h"
#include "curl_error.h"

#include "core/core.h"
#include "response.h"
#include "curl_easy.h"

namespace request
{

struct SocketMon;
struct SocketMonInner;

class CurlMulti : NoCopy {
    friend struct SocketMonInner;

public:
    CurlMulti(asio::any_io_executor ex) noexcept;
    ~CurlMulti();

    auto& get_executor() { return m_ex; }
    auto& get_strand() { return m_strand; }

    template<typename CompletionToken>
    auto async_perform(CurlEasy& easy, CompletionToken&& token) {
        return asio::async_initiate<CompletionToken, void(asio::error_code)>(
            [this](auto&& handler, CurlEasy& easy) {
                asio::dispatch(get_strand(), [this, &easy, handler = std::move(handler)]() mutable {
                    auto      ex = asio::get_associated_executor(handler, get_strand());
                    CURLMcode rc;
                    do {
                        auto ex = asio::get_associated_executor(handler, get_strand());
                        easy.setopt(CURLOPT_SHARE, m_share);
                        if (rc = add_handle(easy); rc != CURLM_OK) break;
                    } while (false);
                    asio::post(ex, std::bind(std::move(handler), ::make_error_code(rc)));
                });
            },
            token,
            std::ref(easy));
    }

    CURLMcode add_handle(CurlEasy&);
    CURLMcode remove_handle(CurlEasy&);

    void load_cookie(std::filesystem::path);
    void save_cookie(std::filesystem::path) const;

private:
    static int static_socket_callback(CURL* easy, curl_socket_t s, int what, void* userp,
                                      SocketMon* socketp);
    static int static_timer_callback(CURLM* multi, long time_ms, void* userp);

    static void static_share_lock(CURL* handle, curl_lock_data data, curl_lock_access access,
                                  void* clientp);
    static void static_share_unlock(CURL* handle, curl_lock_data data, void* clientp);

    int socket_callback(CURL* easy, curl_socket_t s, int what, SocketMon* socketp);

    void asio_socket_callback(const asio::error_code& ec, curl_socket_t s, int what,
                              weak<SocketMonInner>);
    int  socket_action(curl_socket_t, int event_bitmap);

private:
    CURLM*  m_multi;
    CURLSH* m_share;

    asio::any_io_executor               m_ex;
    asio::strand<asio::any_io_executor> m_strand;
    asio::steady_timer                  m_timer;
    std::mutex                          m_share_mutex;
};
} // namespace request
