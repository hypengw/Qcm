#pragma once

#include <atomic>
#include <asio/streambuf.hpp>
#include <asio/any_completion_handler.hpp>

#include "curl_error.h"
#include "curl_easy.h"
#include "session.h"

#include "core/str_helper.h"

namespace request
{
class Session;
class Connection : public std::enable_shared_from_this<Connection> {
    friend Session;

public:
    using executor_type = asio::strand<asio::thread_pool::executor_type>;
    static constexpr usize RECV_LIMIT { 512 * 1024 }; // 0.5M
    enum class State
    {
        NotStarted,
        Transfering,
        Canceled,
        Finished,
    };

    class Buffer {
    public:
        Buffer(usize limit): m_full(false), m_limit(limit) {}
        bool is_full() const { return m_full; }

        auto size() const { return m_buf.size(); };
        auto data() const { return m_buf.data(); };

        auto commit(asio::const_buffer in) {
            auto copied = asio::buffer_copy(m_buf.prepare(in.size()), in);
            m_buf.commit(copied);
            check_full();
            return copied;
        }

        auto consume(asio::mutable_buffer out) {
            auto copied = asio::buffer_copy(out, m_buf.data());
            m_buf.consume(copied);
            check_full();
            return copied;
        }

    private:
        void check_full() { m_full = size() > m_limit; }

        asio::streambuf   m_buf;
        std::atomic<bool> m_full;
        usize             m_limit;
    };

    Connection(executor_type::inner_executor_type ex, rc<Session::channel_type> session_channel)
        : m_finish_ec(CURLE_OK),
          m_state(State::NotStarted),
          m_recv_paused(false),
          m_ex(ex),
          m_easy(std::make_unique<CurlEasy>()),
          m_session_channel(session_channel),
          m_recv_buf(RECV_LIMIT) {
        auto& easy = *m_easy;
        easy.setopt(CURLOPT_WRITEFUNCTION, Connection::write_callback);
        easy.setopt(CURLOPT_WRITEDATA, this);

        easy.setopt(CURLOPT_HEADERFUNCTION, Connection::header_callback);
        easy.setopt(CURLOPT_HEADERDATA, this);

        // easy.setopt(CURLOPT_READFUNCTION, Response::Private::read_callback);
        // easy.setopt(CURLOPT_READDATA, this);
        easy.setopt(CURLOPT_PRIVATE, this);
    }
    ~Connection() {}
    auto  get_rc() { return shared_from_this(); }
    auto& get_executor() { return m_ex; }

    auto& easy() { return *m_easy; }
    auto& easy() const { return *m_easy; }
    auto& channel() { return m_session_channel; }

    auto& header() const { return m_header; }
    auto& url() const { return m_url; }
    void  set_url(std::string_view v) { m_url = v; }

    void about_to_pause(bool v) {
        using Act = session_message::ConnectAction::Action;
        auto msg  = session_message::ConnectAction {
             .con    = get_rc(),
             .action = v ? Act::Pause : Act::UnPause,
        };
        m_session_channel->try_send(asio::error_code {}, msg);
    }

    void about_to_cancel() {
        auto state = m_state.load();
        if (state == State::Canceled || state == State::Finished) return;

        auto msg = session_message::ConnectAction {
            .con    = get_rc(),
            .action = session_message::ConnectAction::Action::Cancel,
        };
        m_session_channel->try_send(asio::error_code {}, msg);
    }

    template<typename Handler>
    void async_read_some(asio::mutable_buffer buf, Handler&& handler) {
        asio::dispatch(m_ex, [this, buf, handler = std::move(handler)]() mutable {
            m_read_some_handler =
                [this, buf, handler = std::move(handler)](asio::error_code ec) mutable {
                    auto copied = m_recv_buf.consume(buf);
                    handler(ec, copied);
                };
            try_read_some_handler();
        });
    }

    using ret_header = void(asio::error_code, Header);
    template<typename CompletionToken>
    auto async_wait_header(CompletionToken&& token) {
        return asio::async_initiate<CompletionToken, void(asio::error_code)>(
            [this](auto&& handler) {
                asio::dispatch(m_ex, [this, handler = std::move(handler)]() mutable {
                    m_wait_header_handler = std::move(handler);
                });
            },
            token);
    }

private:
    static std::size_t header_callback(char* ptr, std::size_t size, std::size_t nmemb,
                                       Connection* self) {
        std::string_view header { ptr, size * nmemb };
        if (! header.empty()) {
            if (auto pos = header.find_first_of(':'); pos != std::string_view::npos) {
                auto iter  = header.begin() + pos;
                auto name  = helper::trims(std::string_view { header.begin(), iter });
                auto value = helper::trims(std::string_view { iter + 1, header.end() });

                if (helper::starts_with_i(name, "set-cookie")) {
                    self->m_cookie_jar.raw_cookie.append(header).push_back('\n');
                } else {
                    self->m_header.insert({ std::string { name }, std::string { value } });
                }
            }
        }
        if (header == "\r\n" || header == "\n") {
            self->try_wait_header_handler();
        }
        return header.size();
    }
    static std::size_t write_callback(char* ptr, std::size_t size, std::size_t nmemb,
                                      Connection* self) {
        auto total_size = size * nmemb;

        auto vec_buf = std::vector<char>(total_size);
        std::copy(ptr, ptr + total_size, vec_buf.begin());

        if (self->m_recv_buf.is_full()) {
            self->m_recv_paused = true;
            return CURL_WRITEFUNC_PAUSE;
        } else {
            asio::dispatch(self->m_ex, [self, in = std::move(vec_buf)]() {
                self->try_wait_header_handler();
                self->m_recv_buf.commit(asio::buffer(in));
                self->try_read_some_handler();
            });

            return total_size;
        }
    }
    static std::size_t read_callback(char* ptr, std::size_t size, std::size_t nmemb,
                                     Connection* self);

    // need to remove easy handler after
    void finish(CURLcode ec) {
        auto self = get_rc();

        // after write_callback and make callback alive
        asio::post(m_ex, [this, self, ec]() {
            m_finish_ec = ec;
            m_state     = State::Finished;
            try_read_some_handler();
            try_wait_header_handler();
        });
    }

    // need to remove easy handler after
    void cancel() {
        auto self = get_rc();

        auto state = m_state.load();
        if (state != State::Finished && state != State::Canceled) {
            m_state = State::Canceled;
        }
        asio::post(m_ex, [this, self]() {
            try_read_some_handler();
            try_wait_header_handler();
        });
    }

    void transfreing() {
        auto self = get_rc();
        asio::post(m_ex, [this, self]() {
            auto state = m_state.load();
            if (state == State::NotStarted) m_state = State::Transfering;
        });
    }

    void try_read_some_handler() {
        if (! m_read_some_handler) return;
        auto recv_size = m_recv_buf.size();
        if (m_state == State::Canceled) {
            DEBUG_LOG("cancel {}", url());
            m_read_some_handler(asio::error::operation_aborted);
        } else if (recv_size > 0) {
            m_read_some_handler(asio::error_code {});
            bool pause { true };
            if (m_recv_buf.size() == 0 && m_recv_paused.compare_exchange_strong(pause, false)) {
                about_to_pause(false);
            }
        } else if (m_state == State::Finished) {
            asio::error_code ec { asio::error::eof };
            if (m_finish_ec != CURLE_OK) ec = m_finish_ec;
            m_read_some_handler(ec);
        }
    }

    void try_wait_header_handler() {
        if (! m_wait_header_handler) return;
        std::error_code ec {};
        if (m_state == State::Canceled) {
            ec = asio::error::operation_aborted;
        }
        m_wait_header_handler(ec);
    }

    std::string m_url;

    CURLcode           m_finish_ec;
    std::atomic<State> m_state;
    std::atomic<bool>  m_recv_paused;

    executor_type             m_ex;
    up<CurlEasy>              m_easy;
    rc<Session::channel_type> m_session_channel;

    Header                                               m_header;
    CookieJar                                            m_cookie_jar;
    asio::any_completion_handler<void(asio::error_code)> m_wait_header_handler;

    Buffer                                               m_recv_buf;
    asio::any_completion_handler<void(asio::error_code)> m_read_some_handler;
};

} // namespace request
