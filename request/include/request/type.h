#pragma once

#include <string_view>
#include <map>
#include <variant>

#include "core/core.h"

namespace request
{

enum class Attribute
{
    HttpCode
};

namespace detail
{

template<Attribute A>
struct attr_type {};

#define AT(A, T)                     \
    template<>                       \
    struct attr_type<Attribute::A> { \
        using type = T;              \
    }

AT(HttpCode, i32);

#undef AT
} // namespace detail

template<Attribute A>
using attr_type = typename detail::attr_type<A>::type;

using attr_value = std::variant<std::monostate, i32, bool>;

enum class Operation
{
    GetOperation,
    PostOperation
};

struct CaseInsensitiveCompare {
    using is_transparent = void;
    bool operator()(std::string_view, std::string_view) const noexcept;
};

using Header = std::map<std::string, std::string, CaseInsensitiveCompare>;

class UrlParams {
public:
    std::string_view param(std::string_view) const;
    UrlParams&       set_param(std::string_view, std::string_view);
    void             decode(std::string_view);
    std::string      encode() const;

private:
    std::map<std::string, std::string, std::less<>> m_params;
};

std::string url_encode(std::string_view);

struct Cookie {};
struct CookieJar {
    std::string raw_cookie;
};

class Url {
public:
    static Url from(std::string_view);

    std::string url;
    std::string host;
    std::string scheme;
    std::string user;
    std::string password;
    std::string port;
    std::string path;
    std::string query;
    std::string fragment;
};

} // namespace request
