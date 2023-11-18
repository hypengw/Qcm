#pragma once

#include <string_view>
#include <map>

#include <asio/experimental/concurrent_channel.hpp>

#include "core/variant_helper.h"
#include "core/core.h"

#include "request/uri.h"

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
std::string url_decode(std::string_view);

struct Cookie {};
struct CookieJar {
    std::string raw_cookie;
};

class Connection;
namespace session_message
{
struct Stop {};

struct ConnectAction {
    enum class Action
    {
        Add,
        Cancel,
        Pause,
        UnPause,
    };
    rc<Connection> con;
    Action         action;
};

using msg = std::variant<Stop, ConnectAction>;
} // namespace session_message

using SessionMessage = session_message::msg;
} // namespace request
