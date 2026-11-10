module;
#include <rstd/macro.hpp>
export module qcm.backend:playback;
export import rstd;
export import :error;
export import :provider.runtime;
export import :resource.service;
import :resource.object_fetch;
import :db.database;
import :db.query.media;
import :db.sync.run;
import rstd.json;
import rstd.log;
import lihttpto;
using alloc::rc::make_rc;
using alloc::rc::Rc;
using rstd::async::AbortOnDropHandle;
using rstd::async::coro;
using rstd::async::OwnedSemaphorePermit;
using rstd::async::Semaphore;
using rstd::sync::Arc;
using rstd::time::Duration;
using rstd::time::Instant;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
struct PlaybackSource {
    String         url;
    Option<String> session;
    bool           time_seekable {};
    Option<f64>    duration_seconds;
};
class PlaybackSessions {
    struct State {
        sqlito::ExecutorHandle db_;
        Rc<ProviderRuntime>    provider_runtime_;
        ResourceObjectFetch    object_fetch_;
        State(sqlito::ExecutorHandle db, Rc<ProviderRuntime> providers)
            : db_(rstd::move(db)), provider_runtime_(rstd::move(providers)) {}

        struct PendingPlaybackResource {
            String                           uri;
            resource_cache::PreparedResource preparation;
        };
        struct PendingPlaybackResponse {
            String                           uri;
            Option<lihttpto::HeaderValue>    range;
            i64                              touched {};
            resource_cache::ResourceDelivery delivery;
        };
        struct PlaybackSessionState {
            Option<OwnedSemaphorePermit> permit;
            String                       token;
            i64                          provider_id {};
            ProviderHandle               provider;
            u64                          epoch {};
            String                       state;
            String                       representation;
            Vec<String>                  origins;
            Vec<PendingPlaybackResource> pending;
            Vec<PendingPlaybackResponse> pending_responses;
            PlaybackLifetime             lifetime { PlaybackLifetime::make() };
            i64                          touched {};
            bool                         closing {};
            auto                         valid() const -> bool {
                if (token.is_empty() || ! provider || ! lifetime->active()) return false;
                auto current = provider->session_epoch();
                return current.is_some() && *current == epoch;
            }
        };
        using PlaybackSession = Rc<PlaybackSessionState>;
        Vec<PlaybackSession> playback_sessions_;
        Semaphore            playback_slots_ { usize(16) };
        struct PlaybackCleanup {
            ProviderHandle           provider;
            AbortOnDropHandle<empty> task;
        };
        Vec<PlaybackCleanup> playback_cleanup_tasks_;
        bool                 playback_stopping_ {};

        static auto find_header(const lihttpto::Headers& headers, ref<str> name)
            -> Option<lihttpto::HeaderValue> {
            auto value = headers.get(name);
            return value.is_some() ? Some((*value)->clone()) : None<lihttpto::HeaderValue>();
        }
        static auto set_payload_header(resource_cache::Response& payload, ref<str> name,
                                       ref<str> value) -> void {
            (void)payload.headers.remove(name);
            payload.headers.add(name, value).unwrap();
        }
        static auto redirect_payload(ref<str> route) -> resource_cache::Response {
            resource_cache::Response payload;
            payload.status = i32(307);
            set_payload_header(payload, "location"_str, route);
            set_payload_header(payload, "content-length"_str, "0"_str);
            set_payload_header(payload, "cache-control"_str, "no-store"_str);
            return payload;
        }

        static auto playback_url(ref<str> uri) -> Option<lihttpto::Url> {
            auto url = lihttpto::Url::parse_http(uri);
            if (url.is_err() || url->userinfo().is_some() || url->fragment().is_some())
                return None();
            return Some(rstd::move(*url));
        }
        static auto playback_millis() -> i64 {
            return i64(Instant::now().duration_since_epoch().as_millis().to_primitive());
        }
        static auto playback_hook_task(ProviderHandle provider, bool closing, String state,
                                       unsigned attempts) -> coro<empty> {
            for (unsigned attempt = 0; attempt < attempts; ++attempt) {
                auto result = co_await rstd::async::timeout(
                    rstd::async::into_future(
                        closing ? provider.get()->close_playback_async(state.clone())
                                : provider.get()->keep_playback_async(state.clone())),
                    Duration::from_secs(u64(5)));
                if (result.is_ok() && result->is_ok()) co_return empty {};
            }
            if (attempts > 1) rstd::log::warn("playback session cleanup did not complete");
            co_return empty {};
        }
        static auto playback_close_task(PlaybackSession owner) -> coro<empty> {
            co_await playback_hook_task(owner->provider.clone(), true, owner->state.clone(), 3);
            co_return empty {};
        }
        auto schedule_playback_close(PlaybackSession owner) -> void {
            playback_cleanup_tasks_.retain([](const auto& task) {
                return ! task.task.is_finished();
            });
            auto task = rstd::async::spawn_local(playback_close_task(owner.clone()));
            playback_cleanup_tasks_.push(PlaybackCleanup {
                owner->provider.clone(), AbortOnDropHandle<empty> { rstd::move(task) } });
        }
        auto discard_playback(PlaybackSession owner) -> void {
            auto& session = *owner.get();
            if (session.closing) return;
            session.closing = true;
            session.lifetime->cancel();
            session.token = String::make();
            session.pending.clear();
            session.pending_responses.clear();
            if (session.provider) schedule_playback_close(rstd::move(owner));
        }
        auto start_playback_service() -> void { playback_stopping_ = false; }
        auto collect_playback() -> coro<empty> {
            if (playback_stopping_) co_return empty {};
            auto                          now = playback_millis();
            Vec<AbortOnDropHandle<empty>> heartbeats;
            for (auto& owner : playback_sessions_) {
                auto& session = *owner.get();
                if (session.token.is_empty()) continue;
                if (! session.valid() || now - session.touched > i64(120000)) {
                    discard_playback(owner.clone());
                    continue;
                }
                prune_playback_responses(session, now);
                auto task = rstd::async::spawn_local(
                    playback_hook_task(session.provider.clone(), false, session.state.clone(), 1));
                heartbeats.push(AbortOnDropHandle<empty> { rstd::move(task) });
            }
            for (auto& heartbeat : heartbeats) (void)co_await rstd::move(heartbeat);
            playback_sessions_.retain([](const auto& session) {
                return ! session->closing;
            });
            co_return empty {};
        }
        auto stop_playback_service() -> coro<empty> {
            playback_stopping_ = true;
            playback_slots_.close();
            for (auto& session : playback_sessions_) discard_playback(session.clone());
            playback_sessions_.clear();
            auto tasks = rstd::move(playback_cleanup_tasks_);
            playback_cleanup_tasks_.clear();
            for (auto& task : tasks) (void)co_await rstd::move(task.task);
            co_return empty {};
        }
        auto cancel_playback_service() -> void {
            playback_stopping_ = true;
            playback_slots_.close();
            for (auto& owner : playback_sessions_) {
                auto& session = *owner.get();
                session.lifetime->cancel();
            }
        }
        auto close_playback(ref<str> token) -> void {
            for (auto& owner : playback_sessions_) {
                auto& session = *owner.get();
                if (session.token != token) continue;
                discard_playback(owner.clone());
                return;
            }
        }
        auto touch_playback(ref<str> token) -> bool {
            for (auto& owner : playback_sessions_) {
                auto& session = *owner.get();
                if (session.token == token) {
                    if (! session.valid()) {
                        discard_playback(owner.clone());
                        return false;
                    }
                    session.touched = playback_millis();
                    return true;
                }
            }
            return false;
        }
        static auto playback_route(PlaybackSessionState& session, ref<str> uri) -> Option<String> {
            auto url = playback_url(uri);
            if (url.is_none()) return None();
            auto origin_value = url->origin();
            if (origin_value.is_err() || origin_value->is_opaque()) return None();
            auto          origin = origin_value->serialize();
            auto          path   = url->request_target();
            Option<usize> index;
            for (usize at {}; at < session.origins.len(); ++at)
                if (session.origins[at] == origin) {
                    index = Some(at);
                    break;
                }
            auto at = usize();
            if (index.is_some()) {
                at = *index;
            } else {
                if (session.origins.len() >= usize(8)) return None();
                at = session.origins.len();
                session.origins.push(rstd::move(origin));
            }
            return Some(rstd::format(
                "/playback/{}/r/{}{}", session.token, at.to_primitive(), path.as_str()));
        }
        static auto playback_same_range(const Option<lihttpto::HeaderValue>& left,
                                        const Option<lihttpto::HeaderValue>& right) -> bool {
            if (left.is_none() || right.is_none()) return left.is_none() && right.is_none();
            return left->as_slice() == right->as_slice();
        }
        static auto prune_playback_responses(PlaybackSessionState& session, i64 now) -> void {
            for (usize index {}; index < session.pending_responses.len();) {
                if (now - session.pending_responses[index].touched < i64(15000)) {
                    ++index;
                    continue;
                }
                auto response = session.pending_responses.remove(index);
            }
        }
        auto playback_session(ref<str> token) -> PlaybackSession {
            for (const auto& session : playback_sessions_)
                if (session->token == token) return session.clone();
            return {};
        }
        static auto prepare_playback_route(PlaybackSession owner, String uri)
            -> coro<Result<Option<String>, ProcessError>> {
            auto& session = *owner.get();
            if (! session.valid()) co_return Err(ProcessError::NotAuth());
            auto url = playback_url(uri.as_str());
            if (url.is_none()) co_return Ok(None<String>());
            auto              canonical = String::make(url->as_ref());
            auto              provider  = session.provider.clone();
            auto              state     = session.state.clone();
            lihttpto::Headers headers;
            auto prepared = rstd_co_try(co_await provider.get()->prepare_playback_resource_async(
                state.as_str(), canonical.as_str(), headers));
            if (! session.valid()) co_return Err(ProcessError::NotAuth());
            auto route = playback_route(session, canonical.as_str());
            if (route.is_none()) co_return Ok(None<String>());
            if (session.pending.len() >= usize(16)) session.pending.clear();
            session.pending.retain([&](const auto& pending) {
                return pending.uri != canonical;
            });
            session.pending.push(
                PendingPlaybackResource { rstd::move(canonical), rstd::move(prepared) });
            co_return Ok(rstd::move(route));
        }
        auto playback_resolve(String token, String uri)
            -> coro<Result<Option<String>, ProcessError>> {
            auto owner = playback_session(token.as_str());
            if (! owner) co_return Ok(None<String>());
            auto& session = *owner.get();
            if (! session.valid()) {
                discard_playback(owner.clone());
                co_return Err(ProcessError::NotAuth());
            }
            session.touched = playback_millis();
            co_return co_await prepare_playback_route(rstd::move(owner), rstd::move(uri));
        }
        struct PlaybackResolution {
            String                                   token, uri, representation;
            Option<lihttpto::HeaderValue>            range;
            u64                                      epoch {};
            Option<PlaybackLifetime>                 lifetime;
            Option<ProviderResourcePlan>             plan;
            Option<resource_cache::ResourceDelivery> immediate;
        };
        static auto immediate_playback(PlaybackSessionState&            session,
                                       resource_cache::ResourceDelivery delivery)
            -> PlaybackResolution {
            PlaybackResolution result;
            result.token     = session.token.clone();
            result.lifetime  = Some(session.lifetime.clone());
            result.immediate = Some(rstd::move(delivery));
            return result;
        }
        auto prepare_playback(ref<str> token, usize index, ref<str> path, ref<str> query,
                              const lihttpto::Headers& headers)
            -> coro<Result<Option<PlaybackResolution>, ProcessError>> {
            auto owner = playback_session(token);
            if (! owner) co_return Ok(None<PlaybackResolution>());
            auto& session = *owner.get();
            if (! session.valid()) {
                discard_playback(owner.clone());
                co_return Err(ProcessError::NotAuth());
            }
            if (index >= session.origins.len()) co_return Ok(None<PlaybackResolution>());
            session.touched   = playback_millis();
            auto range_header = find_header(headers, "range"_str);
            auto uri          = String::make(session.origins[index]);
            uri.push_str(path);
            if (! query.is_empty()) {
                uri.push_str("?"_str);
                uri.push_str(query);
            }
            auto parsed_uri = playback_url(uri.as_str());
            if (parsed_uri.is_none()) co_return Ok(None<PlaybackResolution>());
            uri      = String::make(parsed_uri->as_ref());
            auto now = playback_millis();
            prune_playback_responses(session, now);
            for (usize at {}; at < session.pending_responses.len(); ++at) {
                const auto& pending = session.pending_responses[at];
                if (pending.uri != uri || ! playback_same_range(pending.range, range_header))
                    continue;
                auto response = session.pending_responses.remove(at);
                session.pending.retain([&](const auto& prepared) {
                    return prepared.uri != uri;
                });
                co_return Ok(Some(immediate_playback(session, rstd::move(response.delivery))));
            }
            Option<ProviderResourcePlan> plan;
            for (usize at {}; at < session.pending.len(); ++at) {
                if (session.pending[at].uri != uri) continue;
                auto pending  = session.pending.remove(at);
                auto prepared = rstd::move(pending.preparation);
                rstd_co_try(prepared.apply_headers(headers));
                plan =
                    Some(ProviderResourcePlan { .provider    = session.provider.clone(),
                                                .preparation = rstd::move(prepared),
                                                .provider_id = session.provider_id,
                                                .kind        = resource_cache::ResourceKind::Audio,
                                                .representation = session.representation.clone() });
                break;
            }
            if (plan.is_none()) {
                auto provider = session.provider.clone();
                auto state    = session.state.clone();
                auto prepared =
                    rstd_co_try(co_await provider.get()->prepare_playback_resource_async(
                        state.as_str(), uri.as_str(), headers));
                if (! session.valid()) co_return Err(ProcessError::NotAuth());
                plan =
                    Some(ProviderResourcePlan { .provider    = rstd::move(provider),
                                                .preparation = rstd::move(prepared),
                                                .provider_id = session.provider_id,
                                                .kind        = resource_cache::ResourceKind::Audio,
                                                .representation = session.representation.clone() });
            }
            PlaybackResolution result;
            result.token          = session.token.clone();
            result.uri            = rstd::move(uri);
            result.representation = session.representation.clone();
            result.range          = rstd::move(range_header);
            result.epoch          = session.epoch;
            result.lifetime       = Some(session.lifetime.clone());
            result.plan           = rstd::move(plan);
            co_return Ok(Some(rstd::move(result)));
        }
        auto finish_playback(PlaybackResolution               resolution,
                             resource_cache::ResourceDelivery delivery)
            -> coro<Result<resource_cache::ResourceDelivery, ProcessError>> {
            auto owner = playback_session(resolution.token.as_str());
            if (! owner) co_return Err(ProcessError::NotAuth());
            auto& session = *owner.get();
            if (session.token != resolution.token || resolution.lifetime.is_none() ||
                ! PlaybackLifetime::ptr_eq(session.lifetime, *resolution.lifetime))
                co_return Err(ProcessError::NotAuth());
            if (! session.valid()) {
                discard_playback(owner.clone());
                co_return Err(ProcessError::NotAuth());
            }
            if (delivery.is_LocalFile()) co_return Ok(rstd::move(delivery));
            auto& response     = delivery.as_Body().response;
            auto& uri          = resolution.uri;
            auto& range_header = resolution.range;
            if (response.status >= i32(200) && response.status < i32(300) &&
                response.effective_url.is_some() && *response.effective_url != uri) {
                auto effective = playback_url(response.effective_url->as_str());
                if (effective.is_none())
                    co_return Err(ProcessError::resource(
                        resource_cache::Error::state("invalid playback effective URL"_str)));
                auto target = String::make(effective->as_ref());
                auto route =
                    rstd_co_try(co_await prepare_playback_route(owner.clone(), target.clone()));
                if (! session.valid()) co_return Err(ProcessError::NotAuth());
                if (route.is_none()) co_return Err(ProcessError::NotFound());
                auto now = playback_millis();
                prune_playback_responses(session, now);
                for (usize at {}; at < session.pending_responses.len(); ++at)
                    if (session.pending_responses[at].uri == target &&
                        playback_same_range(session.pending_responses[at].range, range_header)) {
                        (void)session.pending_responses.remove(at);
                        break;
                    }
                if (session.pending_responses.len() >= usize(4))
                    co_return Err(ProcessError::resource(
                        resource_cache::Error::state("playback redirect limit"_str)));
                session.pending_responses.push(PendingPlaybackResponse {
                    rstd::move(target), range_header.clone(), now, rstd::move(delivery) });
                co_return Ok(
                    resource_cache::ResourceDelivery::Body(redirect_payload(route->as_str())));
            }
            co_return Ok(rstd::move(delivery));
        }
        auto resolve_playback_async(String token, usize index, String path, String query,
                                    lihttpto::Headers                          headers,
                                    Option<Arc<resource_cache::ResourceCache>> cache)
            -> coro<Result<Option<resource_cache::ResourceDelivery>, ProcessError>> {
            auto resolution = rstd_co_try(co_await prepare_playback(
                token.as_str(), index, path.as_str(), query.as_str(), headers));
            if (resolution.is_none()) co_return Ok(None<resource_cache::ResourceDelivery>());
            if (resolution->immediate.is_some())
                co_return Ok(Some(rstd::move(*resolution->immediate)));
            auto&             plan = *resolution->plan;
            rstd::json::Array identity;
            identity.push(rstd::json::Value::String(resolution->token.clone()));
            identity.push(rstd::json::Value::String(resolution->representation.clone()));
            identity.push(
                rstd::json::Value::Number(rstd::json::Number::from_u64(resolution->epoch)));
            auto scope    = rstd::json::to_string(rstd::json::Value::Array(rstd::move(identity)));
            auto delivery = rstd_co_try(co_await object_fetch_.respond(
                cache.is_some() ? cache->as_ptr().as_raw_ptr() : nullptr,
                rstd::move(plan),
                rstd::move(headers),
                Some(PlaybackObjectScope { rstd::move(scope), resolution->lifetime->clone() })));
            auto finished = rstd_co_try(
                co_await finish_playback(rstd::move(*resolution), rstd::move(delivery)));
            co_return Ok(Some(rstd::move(finished)));
        }

        auto cancel_provider(ProviderHandle provider) -> void {
            if (! provider) return;
            for (auto& session : playback_sessions_)
                if (! session->token.is_empty() && session->provider.get() == provider.get())
                    discard_playback(session.clone());
        }
        auto invalidate(ProviderHandle provider) -> coro<empty> {
            if (! provider) co_return empty {};
            cancel_provider(provider.clone());
            Vec<AbortOnDropHandle<empty>> closing;
            for (usize index {}; index < playback_cleanup_tasks_.len();)
                if (playback_cleanup_tasks_[index].provider.get() == provider.get()) {
                    auto task = playback_cleanup_tasks_.remove(index);
                    closing.push(rstd::move(task.task));
                } else
                    ++index;
            for (auto& task : closing) (void)co_await rstd::move(task);
            co_return empty {};
        }

        auto playback_source(i64 song_id, Option<i32> max_bitrate)
            -> coro<Result<Option<PlaybackSource>, ProcessError>> {
            if (playback_stopping_) co_return Ok(None<PlaybackSource>());
            auto descriptor =
                rstd_co_try(co_await database_run(db_.clone(), [song_id](sqlito::Connection& db) {
                    return audio_resource_descriptor(db, song_id);
                }));
            if (descriptor.is_none() || playback_stopping_) co_return Ok(None<PlaybackSource>());
            auto provider_id = descriptor->provider_id;
            auto native_id   = rstd::move(descriptor->native_id);
            auto now         = playback_millis();
            for (auto& existing : playback_sessions_)
                if (! existing->token.is_empty() &&
                    (! existing->valid() ||
                     (existing->touched != i64() && now - existing->touched > i64(120 * 1000))))
                    discard_playback(existing.clone());
            for (usize at {}; at < playback_sessions_.len();)
                if (playback_sessions_[at]->token.is_empty())
                    (void)playback_sessions_.remove(at);
                else
                    ++at;
            auto instance = provider_runtime_.get()->acquire(provider_id);
            if (! instance) co_return Ok(None<PlaybackSource>());
            auto epoch = instance->session_epoch();
            if (epoch.is_none()) co_return Err(ProcessError::NotAuth());
            auto permit = rstd_co_try(
                playback_slots_.try_acquire_owned(),
                ProcessError::resource(resource_cache::Error::state("playback session limit"_str)));
            Option<u32> bitrate;
            if (max_bitrate.is_some() && *max_bitrate >= i32())
                bitrate = Some(u32(max_bitrate->to_primitive()));
            auto       playback      = rstd_co_try(co_await instance.get()->open_playback_async(
                PlaybackRequest { rstd::move(native_id), bitrate }));
            auto       current_epoch = instance->session_epoch();
            const bool current       = current_epoch.is_some() && *current_epoch == *epoch &&
                                       provider_runtime_.get()->find(provider_id) == instance.get();
            if (playback.kind == PlaybackKind::HlsVod) {
                auto  owner         = make_rc<PlaybackSessionState>();
                auto& session       = *owner.get();
                session.permit      = Some(rstd::move(permit));
                session.provider_id = provider_id;
                session.provider    = instance.clone();
                session.epoch       = *epoch;
                session.state       = rstd::move(playback.state);
                session.touched     = playback_millis();
                auto token          = try_new_instance_id();
                bool duplicate      = token.is_none();
                if (token.is_some())
                    for (const auto& existing : playback_sessions_)
                        if (existing->token == *token) duplicate = true;
                if (duplicate) {
                    discard_playback(owner.clone());
                    co_return Ok(None<PlaybackSource>());
                }
                session.token = rstd::move(*token);
                struct PendingPlayback {
                    State&          state;
                    PlaybackSession owner;
                    bool            registered {};
                    ~PendingPlayback() {
                        if (! registered) state.discard_playback(owner.clone());
                    }
                } pending { *this, owner.clone() };
                if (! current) {
                    discard_playback(owner.clone());
                    co_return Ok(None<PlaybackSource>());
                }
                session.representation = rstd::move(playback.representation);
                auto route             = rstd_co_try(
                    co_await prepare_playback_route(owner.clone(), rstd::move(playback.uri)));
                if (route.is_none() || playback_stopping_ ||
                    playback_sessions_.len() >= usize(16) || ! session.valid()) {
                    discard_playback(owner.clone());
                    co_return Ok(None<PlaybackSource>());
                }
                for (const auto& existing : playback_sessions_)
                    if (existing->token == session.token) {
                        discard_playback(owner.clone());
                        co_return Ok(None<PlaybackSource>());
                    }
                auto token_text = session.token.clone();
                playback_sessions_.push(rstd::move(owner));
                pending.registered = true;
                co_return Ok(Some(PlaybackSource { rstd::move(*route),
                                                   Some(rstd::move(token_text)),
                                                   true,
                                                   rstd::move(playback.duration_seconds) }));
            }
            if (! current) co_return Err(ProcessError::NotAuth());
            co_return Ok(Some(PlaybackSource {
                rstd::format("/audio/Song/{}", song_id.to_primitive()), None(), false, None() }));
        }
    };
    Rc<State> state_;

    // Retain state before the returned task is first polled.
    template<class T>
    static auto owned_task(Rc<State> state, coro<T> task) -> coro<T> {
        (void)state;
        co_return co_await rstd::move(task);
    }

public:
    PlaybackSessions(sqlito::ExecutorHandle db, Rc<ProviderRuntime> runtime)
        : state_(make_rc<State>(rstd::move(db), rstd::move(runtime))) {}
    PlaybackSessions(const PlaybackSessions&)                    = delete;
    auto operator=(const PlaybackSessions&) -> PlaybackSessions& = delete;
    ~PlaybackSessions() { cancel(); }

    auto start() -> void { state_.get()->start_playback_service(); }
    auto collect() -> coro<empty> {
        return owned_task(state_.clone(), state_.get()->collect_playback());
    }
    auto stop() -> coro<empty> {
        return owned_task(state_.clone(), state_.get()->stop_playback_service());
    }
    auto cancel() -> void { state_.get()->cancel_playback_service(); }
    auto cancel(ProviderHandle provider) -> void {
        state_.get()->cancel_provider(rstd::move(provider));
    }
    auto invalidate(ProviderHandle provider) -> coro<empty> {
        return owned_task(state_.clone(), state_.get()->invalidate(rstd::move(provider)));
    }
    auto close(ref<str> token) -> void { state_.get()->close_playback(token); }
    auto touch(ref<str> token) -> bool { return state_.get()->touch_playback(token); }
    auto source(i64 song_id, Option<i32> max_bitrate = None())
        -> coro<Result<Option<PlaybackSource>, ProcessError>> {
        return owned_task(state_.clone(), state_.get()->playback_source(song_id, max_bitrate));
    }
    auto resolve(String token, String uri) -> coro<Result<Option<String>, ProcessError>> {
        return owned_task(state_.clone(),
                          state_.get()->playback_resolve(rstd::move(token), rstd::move(uri)));
    }
    auto resolve_resource(String token, usize index, String path, String query,
                          lihttpto::Headers                          headers,
                          Option<Arc<resource_cache::ResourceCache>> cache)
        -> coro<Result<Option<resource_cache::ResourceDelivery>, ProcessError>> {
        return owned_task(state_.clone(),
                          state_.get()->resolve_playback_async(rstd::move(token),
                                                               index,
                                                               rstd::move(path),
                                                               rstd::move(query),
                                                               rstd::move(headers),
                                                               rstd::move(cache)));
    }
};
} // namespace qcm::backend
