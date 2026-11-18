export module qcm.backend:playback;
export import rstd;
export import :error;
export import :provider.runtime;
export import :resource.service;
import lihttpto;
using alloc::rc::Rc;
using rstd::async::coro;
using rstd::sync::Arc;
using namespace rstd::prelude;

export namespace qcm::backend
{
struct PlaybackSource {
    String         url;
    Option<String> session;
    bool           time_seekable {};
    Option<f64>    duration_seconds;
};
class PlaybackSessions {
    struct State;
    Rc<State> state_;

    template<class T>
    static auto owned_task(Rc<State> state, coro<T> task) -> coro<T>;

public:
    PlaybackSessions(sqlito::ExecutorHandle db, Rc<ProviderRuntime> runtime);
    PlaybackSessions(const PlaybackSessions&)                    = delete;
    auto operator=(const PlaybackSessions&) -> PlaybackSessions& = delete;
    ~PlaybackSessions();

    auto start() -> void;
    auto collect() -> coro<empty>;
    auto stop() -> coro<empty>;
    auto cancel() -> void;
    auto cancel(ProviderHandle provider) -> void;
    auto invalidate(ProviderHandle provider) -> coro<empty>;
    auto close(ref<str> token) -> void;
    auto touch(ref<str> token) -> bool;
    auto source(i64 song_id, Option<i32> max_bitrate = None())
        -> coro<Result<Option<PlaybackSource>, ProcessError>>;
    auto resolve(String token, String uri) -> coro<Result<Option<String>, ProcessError>>;
    auto resolve_resource(String token, usize index, String path, String query,
                          lihttpto::Headers                          headers,
                          Option<Arc<resource_cache::ResourceCache>> cache)
        -> coro<Result<Option<resource_cache::ResourceDelivery>, ProcessError>>;
};
} // namespace qcm::backend
