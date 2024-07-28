#pragma once

#include <string_view>
#include <list>

#include "core/core.h"
#include "request/type.h"
#include "request/request_opt.h"

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
    Request&    remove_header(std::string_view name);
    void        set_option(const Header&);

    template<typename T>
    T& get_opt() {
        constexpr auto idx = RequestOpts::index<T>();
        return *(static_cast<T*>(get_opt(idx)));
    }

    template<typename T>
    const T& get_opt() const {
        constexpr auto idx = RequestOpts::index<T>();
        return *(static_cast<const T*>(get_opt(idx)));
    }

    void set_opt(const RequestOpt&);

    const Header& header() const;

private:
    const_voidp get_opt(usize) const;
    voidp       get_opt(usize);

    C_DECLARE_PRIVATE(Request, m_d)
};

} // namespace request
