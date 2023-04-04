#pragma once

#include <asio/thread_pool.hpp>
#include <asio/awaitable.hpp>
#include <asio/buffer.hpp>
#include <asio/strand.hpp>

#include <optional>
#include <filesystem>

#include "request/type.h"

#include "core/core.h"
#include "core/expected_helper.h"

namespace request
{

class Request;
class Response;
class Session : public std::enable_shared_from_this<Session>, NoCopy {
    friend class Request;
    friend class Response;

public:
    using executor_type = asio::thread_pool::executor_type;
    using channel_type =
        asio::experimental::concurrent_channel<asio::strand<executor_type>,
                                               void(asio::error_code, SessionMessage)>;

    class Private;
    Session(executor_type ex);
    ~Session();

    executor_type&               get_executor();
    asio::strand<executor_type>& get_strand();
    auto                         get_rc() { return shared_from_this(); }

    asio::awaitable<std::optional<rc<Response>>> get(const Request&);
    asio::awaitable<std::optional<rc<Response>>> post(const Request&, asio::const_buffer);

    void load_cookie(std::filesystem::path);
    void save_cookie(std::filesystem::path) const;

    void about_to_stop();

    void test();

    channel_type&    channel();
    rc<channel_type> channel_rc();

private:
    asio::awaitable<bool> perform(rc<Response>&);

    C_DECLARE_PRIVATE(Session, m_p)

    up<Private> m_p;
};

} // namespace request
