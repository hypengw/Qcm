#include "curl_multi.h"

#include <iostream>
#include <cstdio>

#include <fmt/format.h>
#include <asio/posix/stream_descriptor.hpp>

#include "curl_easy.h"
#include "core/log.h"

#define LOG_CURLM(ec) ERROR_LOG("curlmcode: {}({})", curl_multi_strerror(ec), (int)ec);
#define LOG_CURLE(ec) ERROR_LOG("curlecode: {}({})", curl_esay_strerror(ec), (int)ec);

namespace
{

#define X(_B_) \
    case _B_: return #_B_
std::string_view curl_poll_bit_str(int bit) {
    switch (bit) {
        X(CURL_POLL_NONE);
        X(CURL_POLL_IN);
        X(CURL_POLL_OUT);
        X(CURL_POLL_INOUT);
        X(CURL_POLL_REMOVE);
    }
    return "";
}

std::string_view curl_select_bit_str(int bit) {
    switch (bit) {
        X(CURL_CSELECT_IN);
        X(CURL_CSELECT_OUT);
        X(CURL_CSELECT_ERR);
        X(CURL_SOCKET_TIMEOUT);
    }
    return "";
}

request::CurlEasy* get_curl_private(CURL* c) {
    request::CurlEasy* easy = nullptr;
    curl_easy_getinfo(c, CURLINFO_PRIVATE, &easy);
    _assert_(easy);
    return easy;
}

} // namespace

namespace request
{
struct SocketMonInner : public std::enable_shared_from_this<SocketMonInner> {
    up<asio::posix::stream_descriptor> native_socket { nullptr };

    ~SocketMonInner() {
        // asio always own descriptor, need release it as actually owned by curl
        native_socket->release();
    }

    bool read_mon { false };
    bool write_mon { false };

    void mon_called(int what) {
        if (what == CURL_CSELECT_IN || what == CURL_CSELECT_ERR) {
            read_mon = false;
        }
        if (what == CURL_CSELECT_OUT || what == CURL_CSELECT_ERR) {
            write_mon = false;
        }
    }

    void mon(CurlMulti* multi, curl_socket_t s, int what);
};

struct SocketMon {
    using Inner = SocketMonInner;

    SocketMon(asio::any_io_executor ex, curl_socket_t s): inner(std::make_shared<Inner>()) {
        inner->native_socket = std::make_unique<asio::posix::stream_descriptor>(ex, s);
    }

    weak<Inner> inner_weak() { return inner; }

    rc<Inner> inner;
};
} // namespace request

using namespace request;

void SocketMonInner::mon(CurlMulti* multi, curl_socket_t s, int what) {
    weak<SocketMonInner> inner_weak = shared_from_this();

    if ((what == CURL_POLL_IN || what == CURL_POLL_INOUT) && ! read_mon) {
        native_socket->async_wait(asio::posix::stream_descriptor::wait_read,
                                  [multi, s, inner_weak](const asio::error_code& ec) {
                                      multi->asio_socket_callback(
                                          ec, s, CURL_CSELECT_IN, inner_weak);
                                  });
        read_mon = true;
    }
    if ((what == CURL_POLL_OUT || what == CURL_POLL_INOUT) && ! write_mon) {
        native_socket->async_wait(asio::posix::stream_descriptor::wait_write,
                                  [multi, s, inner_weak](const asio::error_code& ec) {
                                      multi->asio_socket_callback(
                                          ec, s, CURL_CSELECT_OUT, inner_weak);
                                  });
        write_mon = true;
    }
}

CurlMulti::CurlMulti(asio::any_io_executor ex) noexcept
    : m_multi(curl_multi_init()),
      m_share(curl_share_init()),
      m_ex(ex),
      m_strand(ex),
      m_timer(m_strand) {
    curl_multi_setopt(m_multi, CURLMOPT_SOCKETFUNCTION, CurlMulti::static_socket_callback);
    curl_multi_setopt(m_multi, CURLMOPT_SOCKETDATA, this);

    curl_multi_setopt(m_multi, CURLMOPT_TIMERFUNCTION, CurlMulti::static_timer_callback);
    curl_multi_setopt(m_multi, CURLMOPT_TIMERDATA, this);

    curl_share_setopt(m_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
    curl_share_setopt(m_share, CURLSHOPT_LOCKFUNC, CurlMulti::static_share_lock);
    curl_share_setopt(m_share, CURLSHOPT_UNLOCKFUNC, CurlMulti::static_share_unlock);
    curl_share_setopt(m_share, CURLSHOPT_USERDATA, this);
}

CurlMulti::~CurlMulti() {
    curl_multi_cleanup(m_multi);
    curl_share_cleanup(m_share);
}

CURLMcode CurlMulti::add_handle(CurlEasy& easy) {
    auto cm = curl_multi_add_handle(m_multi, easy.handle());
    if (cm != CURLM_OK) {
        LOG_CURLM(cm);
    }
    return cm;
}

CURLMcode CurlMulti::remove_handle(CurlEasy& easy) {
    auto cm = curl_multi_remove_handle(m_multi, easy.handle());
    return cm;
}

int CurlMulti::socket_callback(CURL* c, curl_socket_t s, int what, SocketMon* socketp) {
    if (what == CURL_POLL_REMOVE) {
        curl_multi_assign(m_multi, s, nullptr);
        delete socketp;
        return CURLM_OK;
    }

    if (socketp == nullptr) {
        socketp = new SocketMon(get_curl_private(c)->get_strand(), s);
        curl_multi_assign(m_multi, s, socketp);
    }

    socketp->inner->mon(this, s, what);
    return CURLM_OK;
}

int CurlMulti::static_socket_callback(CURL* easy, curl_socket_t s, int what, void* userp,
                                      SocketMon* socketp) {
    auto info = static_cast<CurlMulti*>(userp);
    return info->socket_callback(easy, s, what, socketp);
}

int CurlMulti::static_timer_callback(CURLM*, long time_ms, void* userp) {
    auto info = static_cast<CurlMulti*>(userp);
    if (time_ms >= 0) {
        info->m_timer.expires_after(asio::chrono::milliseconds(time_ms));
        info->m_timer.async_wait([info](const asio::error_code& ec) {
            if (! ec) {
                info->socket_action(CURL_SOCKET_TIMEOUT, 0);
            }
        });
    } else {
        info->m_timer.cancel();
    }
    return 0;
}

void CurlMulti::asio_socket_callback(const asio::error_code& ec, curl_socket_t s, int what,
                                     weak<SocketMon::Inner> inner_weak) {
    if (ec == asio::error::operation_aborted) return;
    if (ec) {
        what = CURL_CSELECT_ERR;
        ERROR_LOG("asio error {}({})", ec.message(), ec.value());
    }

    auto running_handles = socket_action(s, what);

    bool fin = running_handles == 0;

    if (fin) {
        m_timer.cancel();
    }

    if (auto inner = inner_weak.lock()) {
        inner->mon_called(what);
        if (! fin) {
            inner->mon(this, s, what);
        }
    }
}

int CurlMulti::socket_action(curl_socket_t s, int event_bitmap) {
    int  running_handles { 0 };
    auto rc = curl_multi_socket_action(m_multi, s, event_bitmap, &running_handles);
    if (rc != CURLM_OK) {
        ERROR_LOG("curlmcode: {}({})", curl_multi_strerror(rc), (int)rc);
        _assert_(false);
    }

    int message_left = 0;
    while (CURLMsg* msg = curl_multi_info_read(m_multi, &message_left)) {
        if (msg->msg != CURLMSG_DONE) continue;
        if (msg->easy_handle == nullptr) continue;

        auto easy = get_curl_private(msg->easy_handle);
        _assert_(easy);
        auto rc = msg->data.result;

        easy->done(rc);
    }
    return running_handles;
}

void CurlMulti::static_share_lock(CURL*, curl_lock_data data, curl_lock_access, void* clientp) {
    auto info = static_cast<CurlMulti*>(clientp);
    if (data == curl_lock_data::CURL_LOCK_DATA_COOKIE) {
        info->m_share_mutex.lock();
    }
}
void CurlMulti::static_share_unlock(CURL*, curl_lock_data data, void* clientp) {
    auto info = static_cast<CurlMulti*>(clientp);
    if (data == curl_lock_data::CURL_LOCK_DATA_COOKIE) {
        info->m_share_mutex.unlock();
    }
}

void CurlMulti::load_cookie(std::filesystem::path p) {
    CurlEasy x { m_ex };
    x.setopt(CURLOPT_SHARE, m_share);
    // append filename
    x.setopt(CURLOPT_COOKIEFILE, p.c_str());
    // actually load
    x.setopt(CURLOPT_COOKIELIST, "RELOAD");
}

void CurlMulti::save_cookie(std::filesystem::path p) const {
    CurlEasy x { m_ex };
    x.setopt(CURLOPT_SHARE, m_share);
    x.setopt(CURLOPT_COOKIEJAR, p.c_str());
    // saved when x cleanup
}
