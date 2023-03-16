#pragma once

#include <asio/any_io_executor.hpp>
#include <asio/awaitable.hpp>
#include <asio/buffer.hpp>

#include <optional>
#include <filesystem>

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
    class Private;
    Session(asio::any_io_executor ex);
    ~Session();

    asio::any_io_executor& get_executor();

    asio::awaitable<std::optional<rc<Response>>> get(const Request&);
    asio::awaitable<std::optional<rc<Response>>> post(const Request&, asio::const_buffer);

    void load_cookie(std::filesystem::path);
    void save_cookie(std::filesystem::path) const;

    void test();

private:
    void done(const rc<Response>&);
    asio::awaitable<bool> perform(const rc<Response>&);

    C_DECLARE_PRIVATE(Session, m_p)

    up<Private> m_p;
};

} // namespace request
