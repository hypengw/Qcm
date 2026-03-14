module;
#include "core/macro.h"
export module qcm.asio;
export import :task;
export import :sync_file;
export import :detached_log;
export import :watch_dog;
export import qcm.core;
export import qcm.helper;
export import asio;

DEFINE_CONVERT(cppstd::vector<byte>, asio::streambuf) {
    out.clear();
    cppstd::transform(asio::buffers_begin(in.data()),
                      asio::buffers_end(in.data()),
                      cppstd::back_inserter(out),
                      [](unsigned char c) {
                          return byte { c };
                      });
}

template<>
struct cppstd::formatter<asio::streambuf> : cppstd::formatter<cppstd::string_view> {
    template<typename CTX>
    auto format(const asio::streambuf& buf, CTX& ctx) const -> CTX::iterator {
        cppstd::string out { asio::buffers_begin(buf.data()), asio::buffers_end(buf.data()) };
        return cppstd::formatter<cppstd::string_view>::format(out, ctx);
    }
};

namespace helper
{
template<typename T>
concept is_awaitable = rstd::mtp::spec_of<T, asio::awaitable>;

template<typename T>
concept is_sync_stream = requires(T s, asio::const_buffer buf) {
    { s.write_some(buf) } -> rstd::mtp::convertible_to<rstd::usize>;
};

template<typename Ex, typename ExWork, typename F, typename... Args>
void post_via(const Ex& exec, const ExWork& work_exec, F&& handler, Args&&... args) {
    asio::post(
        exec,
        asio::bind_executor(work_exec,
                            cppstd::bind(rstd::forward<F>(handler), rstd::forward<Args>(args)...)));
}

template<typename Ex, typename F, typename... Args>
void post(const Ex& exec, F&& handler, Args&&... args) {
    post_via(exec,
             asio::get_associated_executor(handler, exec),
             rstd::forward<F>(handler),
             rstd::forward<Args>(args)...);
}
} // namespace helper