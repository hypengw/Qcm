#pragma once

#include <unordered_map>
#include <filesystem>
#include <mutex>
#include <set>
#include <thread>
#include <chrono>

#include <curl/curl.h>
#include <asio/steady_timer.hpp>
#include <asio/thread_pool.hpp>
#include <asio/experimental/channel.hpp>
#include <asio/experimental/concurrent_channel.hpp>

#include "curl_easy.h"
#include "curl_error.h"

#include "core/core.h"
#include "request/response.h"

namespace request
{

class CurlMulti : NoCopy {
public:
    struct InfoMsg {
        CURLMSG  msg;
        CURL*    easy_handle;
        CURLcode result;
    };

    CurlMulti() noexcept: m_multi(curl_multi_init()), m_share(curl_share_init()) {
        // curl_multi_setopt(m_multi, CURLMOPT_SOCKETFUNCTION, CurlMulti::curl_socket_func);
        // curl_multi_setopt(m_multi, CURLMOPT_SOCKETDATA, this);

        // curl_multi_setopt(m_multi, CURLMOPT_TIMERFUNCTION, CurlMulti::curl_timer_func);
        // curl_multi_setopt(m_multi, CURLMOPT_TIMERDATA, this);

        curl_share_setopt(m_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
        curl_share_setopt(m_share, CURLSHOPT_LOCKFUNC, CurlMulti::static_share_lock);
        curl_share_setopt(m_share, CURLSHOPT_UNLOCKFUNC, CurlMulti::static_share_unlock);
        curl_share_setopt(m_share, CURLSHOPT_USERDATA, this);
    }

    ~CurlMulti() {
        curl_multi_cleanup(m_multi);
        curl_share_cleanup(m_share);
    }

    std::error_code add_handle(CurlEasy& easy) {
        std::error_code cm = easy.setopt(CURLOPT_SHARE, m_share);
        if (cm) return cm;
        cm = curl_multi_add_handle(m_multi, easy.handle());
        return cm;
    }

    std::error_code remove_handle(CurlEasy& easy) {
        return curl_multi_remove_handle(m_multi, easy.handle());
    }
    std::error_code remove_handle(CURL* easy) { return curl_multi_remove_handle(m_multi, easy); }

    std::error_code wakeup() { return curl_multi_wakeup(m_multi); }

    std::error_code perform(int& still_running) {
        return curl_multi_perform(m_multi, &still_running);
    }

    std::error_code poll(std::chrono::milliseconds timeout) {
        return curl_multi_poll(m_multi, NULL, 0, (int)timeout.count(), NULL);
    }

    std::vector<InfoMsg> query_info_msg() {
        std::vector<InfoMsg> out;
        int                  message_left { 0 };
        while (CURLMsg* msg = curl_multi_info_read(m_multi, &message_left)) {
            out.push_back(InfoMsg {
                .msg         = msg->msg,
                .easy_handle = msg->easy_handle,
                .result      = msg->data.result,
            });
        }
        return out;
    }

    void load_cookie(std::filesystem::path p) {
        CurlEasy x;
        x.setopt(CURLOPT_SHARE, m_share);
        // append filename
        x.setopt(CURLOPT_COOKIEFILE, p.c_str());
        // actually load
        x.setopt(CURLOPT_COOKIELIST, "RELOAD");
    }

    void save_cookie(std::filesystem::path p) const {
        CurlEasy x;
        x.setopt(CURLOPT_SHARE, m_share);
        x.setopt(CURLOPT_COOKIEJAR, p.c_str());
        // save when x destruct
    }

private:
    static void static_share_lock(CURL*, curl_lock_data data, curl_lock_access, void* clientp) {
        auto info = static_cast<CurlMulti*>(clientp);
        if (data == curl_lock_data::CURL_LOCK_DATA_COOKIE) {
            info->m_share_mutex.lock();
        }
    }

    static void static_share_unlock(CURL*, curl_lock_data data, void* clientp) {
        auto info = static_cast<CurlMulti*>(clientp);
        if (data == curl_lock_data::CURL_LOCK_DATA_COOKIE) {
            info->m_share_mutex.unlock();
        }
    }

private:
    CURLM*  m_multi;
    CURLSH* m_share;

    std::mutex        m_share_mutex;
};
} // namespace request
