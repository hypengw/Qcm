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
    ERR_RET_CO(_RES_, error::Error::expected_chain(helper::to_expected(_R_)))

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

namespace error
{

struct Msg {
    std::string          what;
    std::source_location loc;
};

class Error {
    friend struct fmt::formatter<Error>;

public:
    Error()  = default;
    ~Error() = default;
    Error(std::nullopt_t): Error() {}

    std::string what() const;

    template<typename Fmt>
        requires fmt::formattable<std::decay_t<Fmt>> ||
                 std::same_as<std::decay_t<Fmt>, std::nullopt_t>
    static Error push(Fmt&& f, const std::source_location loc = std::source_location::current()) {
        using T = std::decay_t<Fmt>;
        Msg msg;
        msg.loc = loc;
        if constexpr (std::same_as<Error, T>) {
            f.m_msg_stack.push_back(msg);
            return f;
        } else {
            Error e;
            if constexpr (fmt::formattable<T>) {
                msg.what = fmt::format("{}", f);
            } else {
                msg.what = "nullopt";
            }
            e.m_msg_stack.push_back(msg);
            return e;
        }
    }

    template<typename T, typename E>
    static auto expected_chain(nstd::expected<T, E>&&     exp,
                               const std::source_location loc = std::source_location::current()) {
        return std::move(exp).map_error([&loc](auto err) {
            return Error::push(err, loc);
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
struct fmt::formatter<error::Msg> : fmt::formatter<std::string> {
    template<typename FormatContext>
    auto format(const error::Msg& msg, FormatContext& ctx) const {
        auto out = (fmt::format("{} at {} {}({}:{})",
                                msg.what,
                                msg.loc.function_name(),
                                msg.loc.file_name(),
                                msg.loc.line(),
                                msg.loc.column()));
        return fmt::formatter<std::string>::format(out, ctx);
    }
};

template<>
struct fmt::formatter<error::Error> : fmt::formatter<std::string> {
    template<typename FormatContext>
    auto format(const error::Error& e, FormatContext& ctx) const {
        std::string out { "error:\n" };
        if (e.m_msg_stack.empty()) {
            out.append("    error stack empty");
        } else
            for (auto& msg : e.m_msg_stack) {
                out.append(fmt::format("    {}", msg));
            }
        return fmt::formatter<std::string>::format(out, ctx);
    }
};

inline std::string error::Error::what() const {
    if (m_msg_stack.empty()) return {};
    return m_msg_stack.back().what;
}
