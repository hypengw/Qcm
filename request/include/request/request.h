#pragma once

#include <string_view>
#include <list>

#include "core/core.h"
#include "type.h"

namespace request
{

std::error_code global_init();

class Session;
class Request {
    friend class Session;
    friend class Response;

public:
    class Private;
    Request() noexcept;
    Request(std::string_view url) noexcept;
    ~Request() noexcept;

    Request(const Request&);
    Request& operator=(const Request&);

    std::string_view url() const;
    const URI&       url_info() const;
    Request&         set_url(std::string_view);

    std::string header(std::string_view name) const;
    Request&    set_header(std::string_view name, std::string_view value);
    void        set_option(const Header&);

    const Header& header() const;

    i64      connect_timeout() const;
    Request& set_connect_timeout(i64);

    i64      transfer_timeout() const;
    Request& set_transfer_timeout(i64);

    i64      transfer_low_speed() const;
    Request& set_transfer_low_speed(i64);

    bool     tcp_keepactive() const;
    Request& set_tcp_keepactive(bool);
    i64      tcp_keepidle() const;
    Request& set_tcp_keepidle(i64);
    i64      tcp_keepintvl() const;
    Request& set_tcp_keepintvl(i64);

private:
    C_DECLARE_PRIVATE(Request, m_d)

    up<Private> m_d;
};

} // namespace request
