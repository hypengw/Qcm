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
    requires rstd::mtp::spec_of<T, rstd::result::Result> &&
             requires(T::error_type err, std::source_location loc) {
                 { err.record(loc) };
             }
auto record(T&& exp, const std::source_location loc = std::source_location::current()) {
    return std::forward<T>(exp).map_err([&loc](auto err) {
        return err.record(loc);
    });
}

struct Msg {
    std::string          what;
    std::source_location loc;
};

class Error {
    friend struct rstd::Impl<rstd::fmt::Display, Error>;

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
        requires rstd::mtp::same_as<rstd::mtp::decay<TErr>, Error>
    static Error push(TErr&& err, std::string_view what = {},
                      const std::source_location loc = std::source_location::current()) {
        Msg msg;
        msg.loc  = loc;
        msg.what = what;
        err.m_msg_stack.push_back(msg);
        return err;
    }

    template<typename Fmt>
        requires(! rstd::mtp::same_as<rstd::mtp::decay<Fmt>, Error>) &&
                (rstd::fmt::formattable<std::decay_t<Fmt>, char> ||
                 rstd::mtp::same_as<std::decay_t<Fmt>, std::nullopt_t>)
    static Error push(Fmt&&                         f,
                      const std::source_location loc = std::source_location::current()) {
        using T = rstd::mtp::decay<Fmt>;
        Error e;
        Msg   msg;
        msg.loc = loc;
        if constexpr (rstd::fmt::formattable<T, char>) {
            msg.what = std::format("{}", f);
        } else {
            msg.what = "nullopt";
        }
        e.m_msg_stack.push_back(msg);
        return e;
    }

    // template<typename T>
    //     requires helper::is_expected<T>
    // static auto expected_chain(T&&                        exp,
    //                            const std::source_location loc =
    //                            std::source_location::current()) {
    //     return std::forward<T>(exp).map_error([&loc](auto err) {
    //         if constexpr (std::same_as<std::decay_t<decltype(err)>, Error>) {
    //             return Error::push(err, {}, loc);
    //         } else {
    //             return Error::push(err, loc);
    //         }
    //     });
    // }

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
struct rstd::Impl<rstd::fmt::Display, ::error::Msg> : rstd::ImplBase<::error::Msg> {
    auto fmt(rstd::fmt::Formatter& f) const -> bool {
        auto& msg = this->self();
        auto out = rstd::format("{} at {} {}({}:{})",
                                msg.what,
                                msg.loc.function_name(),
                                msg.loc.file_name(),
                                msg.loc.line(),
                                msg.loc.column());
        return f.write_raw(out.data(), out.size().to_primitive());
    }
};

template<>
struct rstd::Impl<rstd::fmt::Display, std::error_code> : rstd::ImplBase<std::error_code> {
    auto fmt(rstd::fmt::Formatter& f) const -> bool {
        auto& e = this->self();
        auto out = rstd::format("{}({}:{})", e.message(), e.value(), e.category().name());
        return f.write_raw(out.data(), out.size().to_primitive());
    }
};

template<>
struct rstd::Impl<rstd::fmt::Display, ::error::Error> : rstd::ImplBase<::error::Error> {
    auto fmt(rstd::fmt::Formatter& f) const -> bool {
        auto& e = this->self();
        std::string out { "err stack:\n" };
        if (e.m_msg_stack.empty()) {
            out.append("    error stack empty");
        } else {
            std::size_t i { 0 };
            for (auto& msg : e.m_msg_stack) {
                auto s = rstd::format("   {}# {}\n", i++, msg);
                out.append(reinterpret_cast<const char*>(s.as_str().data()),
                           s.size().to_primitive());
            }
        }
        return f.write_raw(out.data(), out.size());
    }
};

inline std::string error::Error::what() const {
    if (m_msg_stack.empty()) return {};
    return m_msg_stack.front().what;
}
