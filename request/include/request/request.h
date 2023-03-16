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
    const Url&       url_info() const;
    Request&         set_url(std::string_view);

    std::string header(std::string_view name) const;
    Request&    set_header(std::string_view name, std::string_view value);
    void        set_option(const Header&);

    const Header& header() const;

    i32      connect_timeout() const;
    Request& set_connect_timeout(i32);

    i32      transfer_timeout() const;
    Request& set_transfer_timeout(i32);

    i32      transfer_low_speed() const;
    Request& set_transfer_low_speed(i32);

private:
    C_DECLARE_PRIVATE(Request, m_d)

    up<Private> m_d;
};

} // namespace request
