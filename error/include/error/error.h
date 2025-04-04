#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <source_location>

#include "core/core.h"
#include "core/fmt.h"
#include "core/expected_helper.h"

#define EC_RET(_RES_, _R_) ERR_RET(_RES_, error::Error::expected_chain(helper::to_expected(_R_)))
#define EC_RET_CO(_RES_, _R_) \
    ERR_RET_CO(_RES_, error::Error::expected_chain(::helper::to_expected(_R_)))

#define OPT_EC_RET(_OPT_)                                             \
    do {                                                              \
        if (auto opt = (_OPT_); opt.has_value())                      \
            return nstd::unexpected(error::Error::push(opt.value())); \
    } while (false)

#define OPT_EC_RET_CO(_OPT_)                                             \
    do {                                                                 \
        if (auto opt = (_OPT_); opt.has_value())                         \
            co_return nstd::unexpected(error::Error::push(opt.value())); \
    } while (false)

#define RECORD(_EXP_) error::record(_EXP_)

namespace error
{

template<typename T>
class ErrorBase : public CRTP<T> {
public:
    T& record(const std::source_location loc = std::source_location::current()) {
        T& err = this->crtp_impl();
        if (static_cast<bool>(err)) {
            m_loc_stack.push_back(loc);
        }
        return err;
    }
    std::span<const std::source_location> location_stack() const { return m_loc_stack; }

private:
    std::vector<std::source_location> m_loc_stack;
};

template<typename T>
    requires nstd::is_expected<T> && requires(T::error_type err, std::source_location loc) {
        { err.record(loc) };
    }
auto record(T&& exp, const std::source_location loc = std::source_location::current()) {
    return std::forward<T>(exp).map_error([&loc](auto err) {
        return err.record(loc);
    });
}

struct Msg {
    std::string          what;
    std::source_location loc;
};

class Error {
    friend struct std::formatter<Error>;
    friend struct std::formatter<Error>;

public:
    Error()  = default;
    ~Error() = default;
    Error(std::nullopt_t): Error() {}
    Error(const Error&)            = default;
    Error& operator=(const Error&) = default;
    Error(Error&& o) noexcept: m_msg_stack(std::move(o.m_msg_stack)) {}
    Error& operator=(Error&& o) noexcept {
        m_msg_stack = std::move(o.m_msg_stack);
        return *this;
    }

    std::string what() const;

    template<typename TErr>
        requires std::same_as<std::decay_t<TErr>, Error>
    static Error push(TErr&& err, std::string_view what = {},
                      const std::source_location loc = std::source_location::current()) {
        Msg msg;
        msg.loc  = loc;
        msg.what = what;
        err.m_msg_stack.push_back(msg);
        return err;
    }

    template<typename Fmt>
        requires(! std::same_as<std::decay_t<Fmt>, Error>) &&
                (std::formattable<std::decay_t<Fmt>, char> ||
                 std::same_as<std::decay_t<Fmt>, std::nullopt_t>)
    static Error push(Fmt&& f, const std::source_location loc = std::source_location::current()) {
        using T = std::decay_t<Fmt>;
        Error e;
        Msg   msg;
        msg.loc = loc;
        if constexpr (std::formattable<T, char>) {
            msg.what = std::format("{}", f);
        } else {
            msg.what = "nullopt";
        }
        e.m_msg_stack.push_back(msg);
        return e;
    }

    template<typename T>
        requires helper::is_expected<T>
    static auto expected_chain(T&&                        exp,
                               const std::source_location loc = std::source_location::current()) {
        return std::forward<T>(exp).map_error([&loc](auto err) {
            if constexpr (std::same_as<std::decay_t<decltype(err)>, Error>) {
                return Error::push(err, {}, loc);
            } else {
                return Error::push(err, loc);
            }
        });
    }

private:
    std::vector<Msg> m_msg_stack;
};

/*
template<typename Fmt>
Error push(Fmt&& f, const std::source_location loc = std::source_location::current()) {
    return Error::push(std::forward<Fmt>(f), loc);
}
*/

} // namespace error

template<>
struct std::formatter<error::Msg> : std::formatter<std::string> {
    template<typename FormatContext>
    auto format(const error::Msg& msg, FormatContext& ctx) const {
        auto out = (std::format("{} at {} {}({}:{})",
                                msg.what,
                                msg.loc.function_name(),
                                msg.loc.file_name(),
                                msg.loc.line(),
                                msg.loc.column()));
        return std::formatter<std::string>::format(out, ctx);
    }
};

template<>
struct std::formatter<std::error_code> : std::formatter<std::string> {
    template<typename FormatContext>
    auto format(const std::error_code& e, FormatContext& ctx) const {
        return std::formatter<std::string>::format(
            std::format("{}({}:{})", e.message(), e.value(), e.category().name()), ctx);
    }
};

template<>
struct std::formatter<error::Error> : std::formatter<std::string> {
    template<typename FormatContext>
    auto format(const error::Error& e, FormatContext& ctx) const {
        std::string out { "err stack:\n" };
        if (e.m_msg_stack.empty()) {
            out.append("    error stack empty");
        } else {
            std::size_t i { 0 };
            for (auto& msg : e.m_msg_stack) {
                out.append(std::format("   {}# {}\n", i++, msg));
            }
        }
        return std::formatter<std::string>::format(out, ctx);
    }
};


inline std::string error::Error::what() const {
    if (m_msg_stack.empty()) return {};
    return m_msg_stack.front().what;
}
