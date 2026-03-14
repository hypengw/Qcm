module;
#include <string>
#include <string_view>
#include <vector>
#include <source_location>

export module qcm.core:error;
import :basic;



export namespace error
{

template<typename T>
class ErrorBase : public CRTP<T> {
public:
    T& record(const cppstd::source_location loc = cppstd::source_location::current()) {
        T& err = this->crtp_impl();
        if (static_cast<bool>(err)) {
            m_loc_stack.push_back(loc);
        }
        return err;
    }
    cppstd::span<const cppstd::source_location> location_stack() const { return m_loc_stack; }

private:
    cppstd::vector<cppstd::source_location> m_loc_stack;
};

template<typename T>
    requires rstd::mtp::spec_of<T, rstd::result::Result> &&
             requires(T::error_type err, cppstd::source_location loc) {
                 { err.record(loc) };
             }
auto record(T&& exp, const cppstd::source_location loc = cppstd::source_location::current()) {
    return cppstd::forward<T>(exp).map_err([&loc](auto err) {
        return err.record(loc);
    });
}

struct Msg {
    cppstd::string          what;
    cppstd::source_location loc;
};

class Error {
    friend struct cppstd::formatter<Error>;
    friend struct cppstd::formatter<Error>;

public:
    Error()  = default;
    ~Error() = default;
    Error(cppstd::nullopt_t): Error() {}
    Error(const Error&)            = default;
    Error& operator=(const Error&) = default;
    Error(Error&& o) noexcept: m_msg_stack(cppstd::move(o.m_msg_stack)) {}
    Error& operator=(Error&& o) noexcept {
        m_msg_stack = cppstd::move(o.m_msg_stack);
        return *this;
    }

    cppstd::string what() const;

    template<typename TErr>
        requires rstd::mtp::same_as<rstd::mtp::decay<TErr>, Error>
    static Error push(TErr&& err, cppstd::string_view what = {},
                      const cppstd::source_location loc = cppstd::source_location::current()) {
        Msg msg;
        msg.loc  = loc;
        msg.what = what;
        err.m_msg_stack.push_back(msg);
        return err;
    }

    template<typename Fmt>
        requires(! rstd::mtp::same_as<rstd::mtp::decay<Fmt>, Error>) &&
                (rstd::fmt::formattable<cppstd::decay_t<Fmt>, char> ||
                 rstd::mtp::same_as<cppstd::decay_t<Fmt>, cppstd::nullopt_t>)
    static Error push(Fmt&&                         f,
                      const cppstd::source_location loc = cppstd::source_location::current()) {
        using T = rstd::mtp::decay<Fmt>;
        Error e;
        Msg   msg;
        msg.loc = loc;
        if constexpr (rstd::fmt::formattable<T, char>) {
            msg.what = cppstd::format("{}", f);
        } else {
            msg.what = "nullopt";
        }
        e.m_msg_stack.push_back(msg);
        return e;
    }

    // template<typename T>
    //     requires helper::is_expected<T>
    // static auto expected_chain(T&&                        exp,
    //                            const cppstd::source_location loc =
    //                            cppstd::source_location::current()) {
    //     return cppstd::forward<T>(exp).map_error([&loc](auto err) {
    //         if constexpr (cppstd::same_as<cppstd::decay_t<decltype(err)>, Error>) {
    //             return Error::push(err, {}, loc);
    //         } else {
    //             return Error::push(err, loc);
    //         }
    //     });
    // }

private:
    cppstd::vector<Msg> m_msg_stack;
};

/*
template<typename Fmt>
Error push(Fmt&& f, const cppstd::source_location loc = cppstd::source_location::current()) {
    return Error::push(cppstd::forward<Fmt>(f), loc);
}
*/

} // namespace error

template<>
struct cppstd::formatter<error::Msg> : cppstd::formatter<cppstd::string> {
    template<typename FormatContext>
    auto format(const error::Msg& msg, FormatContext& ctx) const {
        auto out = (cppstd::format("{} at {} {}({}:{})",
                                   msg.what,
                                   msg.loc.function_name(),
                                   msg.loc.file_name(),
                                   msg.loc.line(),
                                   msg.loc.column()));
        return cppstd::formatter<cppstd::string>::format(out, ctx);
    }
};

template<>
struct cppstd::formatter<cppstd::error_code> : cppstd::formatter<cppstd::string> {
    template<typename FormatContext>
    auto format(const cppstd::error_code& e, FormatContext& ctx) const {
        return cppstd::formatter<cppstd::string>::format(
            cppstd::format("{}({}:{})", e.message(), e.value(), e.category().name()), ctx);
    }
};

template<>
struct cppstd::formatter<error::Error> : cppstd::formatter<cppstd::string> {
    template<typename FormatContext>
    auto format(const error::Error& e, FormatContext& ctx) const {
        cppstd::string out { "err stack:\n" };
        if (e.m_msg_stack.empty()) {
            out.append("    error stack empty");
        } else {
            cppstd::size_t i { 0 };
            for (auto& msg : e.m_msg_stack) {
                out.append(cppstd::format("   {}# {}\n", i++, msg));
            }
        }
        return cppstd::formatter<cppstd::string>::format(out, ctx);
    }
};

inline cppstd::string error::Error::what() const {
    if (m_msg_stack.empty()) return {};
    return m_msg_stack.front().what;
}
