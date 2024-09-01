#pragma once

#include <string_view>
#include <map>

// #include <asio/experimental/concurrent_channel.hpp>

#include "core/core.h"
#include "core/variant_helper.h"

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
    GetOperation    = 0,
    PostOperation   = 1,
    DeleteOperation = 2,
    HeadOperation   = 3,
    GET             = GetOperation,
    POST            = PostOperation,
    DELETE          = DeleteOperation,
    HEAD            = HeadOperation,
};

struct CaseInsensitiveCompare {
    using is_transparent = void;
    bool operator()(std::string_view, std::string_view) const noexcept;
};

using Header = std::map<std::string, std::string, CaseInsensitiveCompare>;

class UrlParams {
public:
    auto param(std::string_view) const -> std::string_view;
    auto params(std::string_view) const -> std::vector<std::string_view>;
    auto is_array(std::string_view) const -> bool;
    auto set_param(std::string_view, std::string_view) -> UrlParams&;
    auto add_param(std::string_view, std::string_view) -> UrlParams&;

    template<typename T>
        requires(! std::convertible_to<T, std::string_view>) && std::ranges::range<T>
    auto set_param(std::string_view name, const T& arr) -> UrlParams& {
        if constexpr (std::same_as<std::string, T> || std::same_as<std::string_view, T>) {
            for (const auto& el : arr) add_param(name, el);
        } else {
            for (const auto& el : arr) add_param(name, convert_from<std::string>(el));
        }
        return *this;
    }
    auto set_param(std::string_view, const std::map<std::string, std::string>&) -> UrlParams& {
        return *this;
    }

    void decode(std::string_view);
    auto encode() const -> std::string;

private:
    std::map<std::string, std::vector<std::string>, std::less<>> m_params;
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
        PauseRecv,
        UnPauseRecv,
        PauseSend,
        UnPauseSend,
    };
    rc<Connection> con;
    Action         action;
};

using msg = std::variant<Stop, ConnectAction>;
} // namespace session_message

using SessionMessage = session_message::msg;

} // namespace request
