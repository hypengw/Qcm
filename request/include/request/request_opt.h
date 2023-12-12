#pragma once

#include <string>
#include <variant>
#include "core/type_list.h"

namespace request
{
namespace req_opt
{
#define PROP(Type, Name, Init)            \
    Type Name Init;                       \
    auto&     set_##Name(const Type& v) { \
        Name = v;                     \
        return *this;                 \
    }

struct Timeout {
    PROP(i64, low_speed, {})
    PROP(i64, connect_timeout, {})
    PROP(i64, transfer_timeout, {})
};

struct Proxy {
    enum class Type
    {
        HTTP    = 0,
        HTTPS2  = 3,
        SOCKS4  = 4,
        SOCKS5  = 5,
        SOCKS4A = 6,
        SOCKS5H = 7
    };
    PROP(Type, type, { Type::HTTP })
    PROP(std::string, content, {})
};

struct Tcp {
    PROP(bool, keepalive, {})
    PROP(i64, keepidle, {})
    PROP(i64, keepintvl, {})
};

using opts = core::type_list<Timeout, Proxy, Tcp>;

} // namespace req_opt

using RequestOpts = req_opt::opts;
using RequestOpt  = RequestOpts::to<std::variant>;

} // namespace request