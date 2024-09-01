#pragma once

#include <atomic>
#include <mutex>
#include <asio/streambuf.hpp>
#include <asio/any_completion_handler.hpp>
#include <asio/experimental/concurrent_channel.hpp>

#include "curl_error.h"
#include "curl_easy.h"
#include "request/session.h"
#include "request/http_header.h"

#include "core/str_helper.h"

namespace request
{
class Session;
class Connection : public std::enable_shared_from_this<Connection> {
    friend Session;

public:
    using executor_type  = asio::strand<asio::thread_pool::executor_type>;
    using allocator_type = std::pmr::polymorphic_allocator<char>;

    static constexpr usize RECV_LIMIT { 512 * 1024 }; // 0.5M
    static constexpr usize SEND_LIMIT { 512 * 1024 }; // 0.5M

    enum class State
    {
        NotStarted,
        Transfering,
        Canceled,
        Finished,
    };

    template<typename Allocator>
    class Buffer {
    public:
        Buffer(usize limit, const Allocator& aloc)
            : m_buf(std::numeric_limits<std::size_t>::max(), aloc),
              m_state(State::Empty),
              m_limit(limit),
              m_transferred(0),
              m_alloc(aloc) {}
        enum class State : i32
        {
            Empty = 0,
            Normal,
            Full,
        };

        bool is_full() const { return m_state == State::Full; }
        bool empty() const { return m_state == State::Empty; }

        auto size() const { return m_buf.size(); };
        auto data() const { return m_buf.data(); };

        auto commit(asio::const_buffer in) {
            auto copied = asio::buffer_copy(m_buf.prepare(in.size()), in);
            m_buf.commit(copied);
            m_transferred += copied;
            check_full();
            return copied;
        }

        auto consume(asio::mutable_buffer out) {
            auto copied = asio::buffer_copy(out, m_buf.data());
            m_buf.consume(copied);
            check_full();
            return copied;
        }

        auto allocator() const { return m_alloc; }

    private:
        void check_full() {
            auto s  = size();
            m_state = s == 0 ? State::Empty : (size() > m_limit ? State::Full : State::Normal);
        }

        asio::basic_streambuf<Allocator> m_buf;
        std::atomic<State>               m_state;
        usize                            m_limit;
        usize                            m_transferred;
        Allocator                        m_alloc;
    };

    Connection(executor_type::inner_executor_type ex, rc<Session::channel_type> session_channel,
               allocator_type allocator)
        : m_finish_ec(CURLE_OK),
          m_state(State::NotStarted),
          m_recv_paused(false),
          m_ex(ex),
          m_easy(std::make_unique<CurlEasy>()),
          m_session_channel(session_channel),
          m_recv_buf(RECV_LIMIT, allocator),
          m_send_buf(SEND_LIMIT, allocator) {
        auto& easy = *m_easy;
        easy.setopt(CURLOPT_WRITEFUNCTION, Connection::write_callback);
        easy.setopt(CURLOPT_WRITEDATA, this);

        easy.setopt(CURLOPT_HEADERFUNCTION, Connection::header_callback);
        easy.setopt(CURLOPT_HEADERDATA, this);

        easy.setopt(CURLOPT_READFUNCTION, Connection::read_callback);
        easy.setopt(CURLOPT_READDATA, this);
        easy.setopt(CURLOPT_PRIVATE, this);
    }
    ~Connection() {}
    auto  get_rc() { return shared_from_this(); }
    auto& get_executor() { return m_ex; }

    auto& easy() { return *m_easy; }
    auto& easy() const { return *m_easy; }
    auto& channel() { return m_session_channel; }

    auto& header() const { return m_header_; }
    auto& url() const { return m_url; }
    void  set_url(std::string_view v) { m_url = v; }
    void  set_send_callback(const req_opt::Read::Callback& cb) { m_send_callback = cb; }

    using Action = session_message::ConnectAction::Action;
    void send_action(Action v) {
        auto msg = session_message::ConnectAction { .con = get_rc(), .action = v };
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

    template<typename Handler>
    void async_write_some(asio::const_buffer buf, Handler&& handler) {
        asio::dispatch(m_ex, [this, buf, handler = std::move(handler)]() mutable {
            m_write_some_handler =
                [this, buf, handler = std::move(handler)](asio::error_code ec) mutable {
                    usize copied { 0 };
                    if (! ec) {
                        std::unique_lock lock { m_send_mutex };
                        copied = m_send_buf.commit(buf);
                    }
                    handler(ec, copied);
                };

            try_write_some_handler();
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
        self->m_header_raw.append(header);
        if (! self->m_header_.start) {
            self->m_header_.start = HttpHeader::parse_start_line(header);
        } else {
            auto field = HttpHeader::parse_field_line(header);
            if (! field.name.empty()) {
                self->m_header_.fields.push_back(field);
                if (helper::starts_with_i(field.name, "set-cookie")) {
                    self->m_cookie_jar.raw_cookie.append(header).push_back('\n');
                }
            }
        }
        if (self->m_header_.start && header == "\r\n") {
            self->try_wait_header_handler();
        }
        return header.size();
    }
    static std::size_t write_callback(char* ptr, std::size_t size, std::size_t nmemb,
                                      Connection* self) {
        auto total_size = size * nmemb;

        auto vec_buf = std::vector<char, allocator_type>(total_size, self->m_recv_buf.allocator());
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
                                     Connection* self) {
        auto total_size = size * nmemb;
        if (self->m_send_callback) {
            return self->m_send_callback((byte*)ptr, total_size);
        } else {
            usize copied { 0 };
            if (self->m_send_buf.empty()) {
                return CURL_READFUNC_PAUSE;
            } else {
                {
                    std::unique_lock lock { self->m_send_mutex };
                    copied = self->m_send_buf.consume(asio::mutable_buffer { ptr, total_size });
                }
                asio::dispatch(self->m_ex, [self]() {
                    self->try_write_some_handler();
                });
            }
            return copied;
        }
    }

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
            try_write_some_handler();
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
                send_action(Action::UnPauseRecv);
            }
        } else if (m_state == State::Finished) {
            asio::error_code ec { asio::error::eof };
            if (m_finish_ec != CURLE_OK) {
                ec = m_finish_ec;
            }
            m_read_some_handler(ec);
        }
    }

    void try_write_some_handler() {
        if (! m_write_some_handler) return;

        if (m_state == State::Canceled) {
            DEBUG_LOG("cancel {}", url());
            m_write_some_handler(asio::error::operation_aborted);
        } else if (! m_send_buf.is_full()) {
            m_write_some_handler(asio::error_code {});
            bool pause { true };
            if (m_send_paused.compare_exchange_strong(pause, false)) {
                send_action(Action::UnPauseSend);
            }
        } else if (m_state == State::Finished) {
            asio::error_code ec { asio::error::eof };
            if (m_finish_ec != CURLE_OK) {
                ec = m_finish_ec;
            }
            m_write_some_handler(ec);
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
    std::atomic<bool>  m_send_paused;

    executor_type             m_ex;
    up<CurlEasy>              m_easy;
    rc<Session::channel_type> m_session_channel;

    std::string                                          m_header_raw;
    HttpHeader                                           m_header_;
    CookieJar                                            m_cookie_jar;
    asio::any_completion_handler<void(asio::error_code)> m_wait_header_handler;

    Buffer<allocator_type>                               m_recv_buf;
    asio::any_completion_handler<void(asio::error_code)> m_read_some_handler;

    req_opt::Read::Callback                              m_send_callback;
    std::mutex                                           m_send_mutex;
    Buffer<allocator_type>                               m_send_buf;
    asio::any_completion_handler<void(asio::error_code)> m_write_some_handler;
};

} // namespace request
