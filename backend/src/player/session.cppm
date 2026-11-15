module;
#include <rstd/macro.hpp>
export module qcm.backend:player.session;
export import :player.process;
export import :player.queue;
export import :player.preferences;
export import :player.integration;
import :api.radio;
import :db.database;
import :db.query.player;
import :db.state.playlog;
import :db.timestamp;
import :event;
import :protocol.message;
import :provider.runtime;
import :uuid;
import rstd.log;
using alloc::rc::Rc;
using rstd::async::AbortOnDropHandle;
using rstd::async::coro;
using rstd::async::mpsc::Receiver;
using rstd::async::mpsc::Sender;
using rstd::collections::BTreeMap;
using rstd::path::PathBuf;
using rstd::sync::Arc;
using rstd::sync::Weak;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace ipc  = qcm::player_ipc;
namespace wire = p_qcm::p_msg;

export namespace qcm::backend
{
class PlayerSession {
    struct PlayerEvent {
        ipc::Event event;
        u64        generation, intent;
    };
    struct PendingAdvance {
        u64  intent, epoch;
        bool ended;
    };
    struct PlaylogEntry {
        i64           id;
        Timestamp     timestamp;
        PlayingAction action;
    };
    sqlito::ExecutorHandle              db_;
    Rc<ProviderRuntime>                 providers_;
    Arc<RadioSessions>                  radios_;
    SyncCoordinator                     coordinator_;
    PathBuf                             program_;
    String                              base_;
    EventSink                           events_;
    Weak<PlayerSession>                 self_ { Weak<PlayerSession>::make() };
    PlayerQueue                         queue_;
    ipc::State                          state_;
    Option<PlayerTrack>                 track_;
    Option<PathBuf>                     preferences_file_;
    PlayerPreferences                   saved_preferences_, configured_preferences_;
    Option<i64>                         source_song_, logged_song_;
    Option<u64>                         source_entry_id_;
    u64                                 logged_epoch_ {};
    u64                                 source_intent_ {}, radio_generation_ {};
    Option<Rc<PlayerProcess>>           process_;
    Option<Box<dyn<PlayerIntegration>>> integration_;
    rstd::async::Mutex<empty>           commands_ { empty {} };
    rstd::async::watch::Sender<u64>     changed_ { u64() };
    Option<AbortOnDropHandle<empty>>    publisher_;
    Option<AbortOnDropHandle<empty>>    prefetch_;
    Option<Sender<PlayerEvent>>         event_input_;
    Option<AbortOnDropHandle<empty>>    event_worker_;
    Option<Sender<PlaylogEntry>>        log_input_;
    Option<AbortOnDropHandle<empty>>    log_worker_;
    Option<PendingAdvance>              pending_advance_;
    Option<u64>                         prefetch_entry_;
    Vec<AbortOnDropHandle<empty>>       radio_closures_;
    u64  state_revision_ {}, queue_revision_ {}, epoch_ {}, process_generation_ {}, command_id_ {};
    u64  sequence_ {}, ended_serial_ {}, seek_serial_ {}, intent_ {};
    bool connected_ {}, process_failed_ {}, stopping_ {}, radio_exhausted_ {}, desired_playing_ {};
    String                   error_, name_, radio_session_;
    i64                      radio_id_ {};
    u64                      radio_version_ {};
    QueuePresentation        radio_presentation_;
    BTreeMap<String, String> radio_configuration_;

    void changed(bool queue = false) {
        ++state_revision_;
        if (queue) ++queue_revision_;
        (void)changed_.send_replace(state_revision_);
    }
    void report_error(String text) {
        error_      = rstd::move(text);
        state_.busy = false;
        changed();
    }
    void save_preferences() {
        if (preferences_file_.is_none()) return;
        PlayerPreferences preferences { state_.volume,
                                        state_.fade_ms,
                                        configured_preferences_.loop_mode,
                                        configured_preferences_.shuffle,
                                        configured_preferences_.random_mode };
        if (preferences == saved_preferences_) return;
        auto result = preferences.save(preferences_file_->as_path());
        if (result.is_err())
            rstd::log::warn("cannot save playback preferences: {}", result.unwrap_err());
        else
            saved_preferences_ = preferences;
    }
    static auto publish(Weak<PlayerSession> weak, rstd::async::watch::Receiver<u64> changes)
        -> coro<empty> {
        u64 state_revision {}, queue_revision {};
        for (;;) {
            if ((co_await changes.changed()).is_err()) co_return empty {};
            auto session = weak.upgrade();
            if (! session || session->stopping_) co_return empty {};
            auto& owner = *session;
            owner.save_preferences();
            if (queue_revision != owner.queue_revision_) {
                queue_revision = owner.queue_revision_;
                if (owner.integration_.is_some())
                    (void)co_await (*owner.integration_)->update_queue(owner.queue_state());
                auto message =
                    qcm_message(i32(), Box<wire::M_PlayerQueueMsg>::make(owner.queue_state()));
                if ((co_await owner.events_.send_message(rstd::move(message))).is_err())
                    rstd::log::warn("cannot publish player queue");
            }
            if (state_revision != owner.state_revision_) {
                state_revision = owner.state_revision_;
                if (owner.integration_.is_some())
                    (void)co_await (*owner.integration_)->update_state(owner.state());
                auto message = qcm_message(i32(), Box<wire::M_PlayerStateMsg>::make(owner.state()));
                if ((co_await owner.events_.send_message(rstd::move(message))).is_err())
                    rstd::log::warn("cannot publish player state");
            }
        }
    }
    static auto receive_events(Weak<PlayerSession> weak, Receiver<PlayerEvent> input)
        -> coro<empty> {
        while (auto pending = co_await input.recv()) {
            auto owner = weak.upgrade();
            if (! owner || owner->stopping_) co_return empty {};
            co_await owner->on_event(
                rstd::move(pending->event), pending->generation, pending->intent);
        }
        co_return empty {};
    }
    void start_event_worker() {
        if (event_worker_.is_some()) return;
        auto channel  = rstd::async::mpsc::channel<PlayerEvent>(usize(64));
        event_input_  = Some(rstd::move(rstd::get<0>(channel)));
        event_worker_ = Some(AbortOnDropHandle<empty> { rstd::async::spawn_local(
            receive_events(self_.clone(), rstd::move(rstd::get<1>(channel)))) });
        auto logs     = rstd::async::mpsc::channel<PlaylogEntry>(usize(64));
        log_input_    = Some(rstd::move(rstd::get<0>(logs)));
        log_worker_   = Some(AbortOnDropHandle<empty> {
            rstd::async::spawn_local(record_logs(db_.clone(),
                                                 providers_.clone(),
                                                 coordinator_.clone(),
                                                 rstd::move(rstd::get<1>(logs)))) });
    }
    auto ensure_process() -> coro<ProcessResult<empty>> {
        if (stopping_) co_return Err(ProcessError::internal("player is stopping"_str));
        if (process_.is_some() && ! process_failed_) co_return Ok(empty {});
        const auto volume = state_.volume;
        const auto fade   = state_.fade_ms;
        start_event_worker();
        auto generation = ++process_generation_;
        if (auto old = process_.take()) co_await (*old)->shutdown();
        if (stopping_) co_return Err(ProcessError::internal("player is stopping"_str));
        auto weak   = self_.clone();
        auto opened = co_await PlayerProcess::start(
            program_.clone(),
            PlayerEventHandler::make(
                [weak = rstd::move(weak), generation](ipc::Event event) -> coro<empty> {
                    if (auto session = weak.upgrade(); session && ! session->stopping_) {
                        auto input  = session->event_input_->clone();
                        auto intent = session->intent_;
                        (void)co_await input.send(
                            PlayerEvent { rstd::move(event), generation, intent });
                    }
                    co_return empty {};
                }));
        if (opened.is_err()) {
            connected_      = false;
            process_failed_ = true;
            report_error(rstd::move(opened).unwrap_err());
            co_return Err(ProcessError::internal(error_.clone()));
        }
        if (stopping_ || generation != process_generation_) {
            co_await (*opened)->shutdown();
            co_return Err(ProcessError::internal("player startup was canceled"_str));
        }
        process_        = Some(rstd::move(*opened));
        auto process    = (*process_).clone();
        connected_      = true;
        process_failed_ = false;
        sequence_       = u64();
        for (auto action : { ipc::Action::Volume, ipc::Action::Fade }) {
            ipc::Command command;
            command.id     = ++command_id_;
            command.action = action;
            command.value =
                action == ipc::Action::Volume ? volume : static_cast<double>(fade.to_primitive());
            auto sent = co_await process->send(rstd::move(command));
            if (sent.is_err()) co_return Err(ProcessError::internal(rstd::move(sent).unwrap_err()));
            if (stopping_ || generation != process_generation_)
                co_return Err(ProcessError::internal("player startup was canceled"_str));
        }
        if (epoch_ != u64()) {
            ipc::Command command;
            command.id       = ++command_id_;
            command.action   = ipc::Action::Source;
            command.epoch    = ++epoch_;
            command.autoplay = false;
            state_.source.clear();
            state_.playback_state = i32(2);
            auto sent             = co_await process->send(rstd::move(command));
            if (sent.is_err()) co_return Err(ProcessError::internal(rstd::move(sent).unwrap_err()));
        }
        changed();
        co_return Ok(empty {});
    }
    auto issue(u64 intent, ipc::Action action, double value = 0, String source = {},
               bool autoplay = true, Option<i64> song = None()) -> coro<ProcessResult<empty>> {
        rstd_co_try(co_await ensure_process());
        if (intent != intent_ || stopping_) co_return Ok(empty {});
        if (action == ipc::Action::Source) {
            ++epoch_;
            source_song_     = song;
            source_entry_id_ = song.is_some() ? queue_.current_entry_id() : None<u64>();
            source_intent_   = intent;
            state_.source    = source.clone();
            state_.busy      = true;
            changed();
        }
        ipc::Command command;
        command.id       = ++command_id_;
        command.epoch    = epoch_;
        command.action   = action;
        command.source   = rstd::move(source);
        command.value    = value;
        command.autoplay = autoplay;
        auto sent        = co_await (*process_)->send(rstd::move(command));
        if (sent.is_err()) {
            report_error(rstd::move(sent).unwrap_err());
            co_return Err(ProcessError::internal(error_.clone()));
        }
        co_return Ok(empty {});
    }
    static auto release_radio(Rc<ProviderRuntime> providers, Arc<RadioSessions> radios,
                              SyncCoordinator coordinator, sqlito::ExecutorHandle db, String id)
        -> coro<empty> {
        co_await radios->close(*providers.get(), coordinator, rstd::move(db), rstd::move(id));
        co_return empty {};
    }
    auto close_radio() -> coro<empty> {
        if (radio_id_ > i64()) {
            (void)queue_.set_loop_mode(
                static_cast<LoopMode>(configured_preferences_.loop_mode.to_primitive()));
            (void)queue_.set_shuffle(configured_preferences_.shuffle);
            (void)queue_.set_random_mode(configured_preferences_.random_mode);
        }
        ++radio_generation_;
        pending_advance_ = None();
        prefetch_entry_  = None();
        radio_id_        = i64();
        radio_version_   = u64();
        radio_exhausted_ = false;
        radio_configuration_.clear();
        radio_presentation_ = QueuePresentation {};
        auto old            = rstd::exchange(radio_session_, String::make());
        if (! old.is_empty()) {
            radio_closures_.retain([](const auto& task) {
                return ! task.is_finished();
            });
            radio_closures_.push(AbortOnDropHandle<empty> {
                rstd::async::spawn_local(release_radio(providers_.clone(),
                                                       radios_.clone(),
                                                       coordinator_.clone(),
                                                       db_.clone(),
                                                       rstd::move(old))) });
        }
        co_return empty {};
    }
    auto clear_source(u64 intent) -> coro<ProcessResult<empty>> {
        if (intent != intent_ || stopping_) co_return Ok(empty {});
        state_.source.clear();
        state_.position_ms = i64();
        state_.duration_ms = i64();
        state_.seekable    = false;
        track_             = None();
        state_.busy        = true;
        error_.clear();
        changed();
        co_return co_await issue(intent, ipc::Action::Source, 0, {}, false);
    }
    auto select_source(u64 intent) -> coro<ProcessResult<empty>> {
        auto id = queue_.current_id();
        if (id.is_none()) co_return co_await clear_source(intent);
        rstd_co_try(co_await ensure_process());
        if (intent != intent_ || stopping_) co_return Ok(empty {});
        auto loaded = co_await database_run(db_.clone(), [id = *id](sqlito::Connection& db) {
            return player_track(db, id);
        });
        if (intent != intent_ || stopping_) co_return Ok(empty {});
        if (loaded.is_err()) {
            auto error = rstd::move(loaded).unwrap_err();
            (void)co_await clear_source(intent);
            report_error(rstd::format("{}", error));
            co_return Err(rstd::move(error));
        }
        if (! loaded->can_play) {
            (void)co_await clear_source(intent);
            report_error("song is not playable"_Str);
            co_return Err(ProcessError::internal(error_.clone()));
        }
        track_             = Some(rstd::move(*loaded));
        state_.source      = rstd::format("{}/playback/source/Song/{}", base_, *id);
        state_.position_ms = i64();
        state_.duration_ms = track_->duration;
        state_.busy        = true;
        state_.seekable    = false;
        error_.clear();
        changed();
        rstd_co_try(co_await issue(
            intent, ipc::Action::Source, 0, state_.source.clone(), desired_playing_, id));
        co_return Ok(empty {});
    }
    void adopt_radio(const p_qcm::p_msg::p_model::M_QueueSession& session) {
        radio_session_       = session.f_id.clone();
        radio_version_       = session.f_version;
        radio_exhausted_     = session.f_exhausted;
        radio_configuration_ = session.f_configuration.clone();
        if (session.f_presentation.is_some()) {
            const auto& presentation           = **session.f_presentation;
            radio_presentation_.description    = presentation.f_description.clone();
            radio_presentation_.source_item_id = presentation.f_source_item_id > i64()
                                                     ? Some(i64(presentation.f_source_item_id))
                                                     : None<i64>();
        }
    }
    auto next_radio() -> coro<ProcessResult<empty>> {
        if (radio_id_ <= i64() || radio_exhausted_ || radio_session_.is_empty())
            co_return Ok(empty {});
        auto                    generation = radio_generation_;
        auto                    session    = radio_session_.clone();
        wire::M_GetQueueNextReq request;
        auto                    id = Box<p_qcm::p_msg::p_id::M_RadioQueueId>::make();
        id->f_value                = radio_id_;
        request.f_queue_id         = Some(rstd::move(id));
        request.f_current_song_ids = Some(Box<p_qcm::p_msg::p_id::M_SongIds>::make());
        for (auto song : queue_.ids())
            (*request.f_current_song_ids)->f_values.push(rstd::move(song));
        request.f_session_id       = radio_session_.clone();
        request.f_expected_version = radio_version_;
        request.f_operation_id     = rstd::format("{}:{}", current_millis(), new_instance_id());
        auto result                = co_await next_queue_async(
            *providers_.get(), *radios_, coordinator_, db_.clone(), i32(), request);
        if (stopping_ || generation != radio_generation_ || session != radio_session_)
            co_return Ok(empty {});
        auto  response = rstd_co_try(rstd::move(result));
        auto& page     = *response.o_payload->as_get_queue_next_rsp();
        if (page.f_session.is_some()) adopt_radio(**page.f_session);
        if (page.f_song_ids.is_some())
            (void)rstd_co_try(queue_.append(rstd::move((*page.f_song_ids)->f_values)),
                              ProcessError::internal(rstd::move(_e)));
        changed(true);
        co_return Ok(empty {});
    }
    static auto prefetch(Weak<PlayerSession> weak) -> coro<empty> {
        if (auto session = weak.upgrade()) {
            auto generation = session->radio_generation_;
            auto result     = co_await session->next_radio();
            auto guard      = co_await session->commands_.lock();
            if (session->stopping_ || generation != session->radio_generation_) co_return empty {};
            if (result.is_err()) {
                session->pending_advance_ = None();
                session->report_error(rstd::format("{}", result.unwrap_err()));
            } else if (auto pending = session->pending_advance_.take()) {
                if (pending->intent == session->intent_ &&
                    (! pending->ended ||
                     (pending->epoch == session->epoch_ && session->desired_playing_))) {
                    auto advanced =
                        co_await session->advance(false, pending->ended, pending->intent, false);
                    if (advanced.is_err())
                        session->report_error(rstd::format("{}", advanced.unwrap_err()));
                }
            }
        }
        co_return empty {};
    }
    void maybe_prefetch(bool force = false) {
        if (stopping_ || radio_id_ <= i64() || radio_exhausted_) return;
        if (! force &&
            (queue_.current_index() < i32() ||
             usize(static_cast<rstd::size_t>(queue_.current_index().to_primitive())) + usize(3) <
                 queue_.ids().len()))
            return;
        if (prefetch_.is_some() && ! prefetch_->is_finished()) return;
        if (! force && prefetch_entry_ == queue_.current_entry_id()) return;
        prefetch_entry_ = queue_.current_entry_id();
        prefetch_ =
            Some(AbortOnDropHandle<empty> { rstd::async::spawn_local(prefetch(self_.clone())) });
    }
    auto advance(bool previous, bool ended, u64 intent, bool fetch = true)
        -> coro<ProcessResult<empty>> {
        if (fetch && ! previous && radio_id_ > i64() && ! radio_exhausted_ &&
            ! queue_.can_next_without_loop() &&
            (! ended || queue_.loop_mode() != LoopMode::Track)) {
            pending_advance_ = Some(PendingAdvance { intent, epoch_, ended });
            maybe_prefetch(true);
            co_return Ok(empty {});
        }
        if (intent != intent_ || stopping_ || (ended && ! desired_playing_)) co_return Ok(empty {});
        auto change = rstd_co_try(previous ? queue_.previous() : queue_.next(ended),
                                  ProcessError::internal(rstd::move(_e)));
        changed(true);
        if (change == PlayerQueueChange::CurrentChanged || change == PlayerQueueChange::Restart)
            rstd_co_try(co_await select_source(intent));
        else if (change == PlayerQueueChange::Exhausted && ended) {
            desired_playing_ = false;
            rstd_co_try(co_await issue(intent, ipc::Action::Stop));
        }
        maybe_prefetch();
        co_return Ok(empty {});
    }
    auto on_event(ipc::Event event, u64 generation, u64 observed_intent) -> coro<empty> {
        if (stopping_ || generation != process_generation_) co_return empty {};
        if (event.kind == ipc::EventKind::Ready) co_return empty {};
        if (event.kind == ipc::EventKind::Error && event.id == u64()) {
            connected_            = false;
            process_failed_       = true;
            state_.playback_state = i32(2);
            state_.seekable       = false;
            report_error(rstd::move(event.state.error));
            if (auto id = logged_song_.take()) co_await log(*id, PlayingAction::Stop);
            co_return empty {};
        }
        if (event.state.epoch != epoch_ || event.state.sequence <= sequence_) co_return empty {};
        sequence_ = event.state.sequence;
        if (event.kind == ipc::EventKind::Ack) co_return empty {};
        if (event.kind == ipc::EventKind::Error) {
            auto error = event.state.error.clone();
            state_     = rstd::move(event.state);
            report_error(rstd::move(error));
            if (state_.playback_state == i32(2))
                if (auto id = logged_song_.take()) co_await log(*id, PlayingAction::Stop);
            co_return empty {};
        }
        state_     = rstd::move(event.state);
        connected_ = true;
        if (event.kind == ipc::EventKind::Seeked) ++seek_serial_;
        if (event.kind == ipc::EventKind::Ended) ++ended_serial_;
        changed();
        auto       observed_epoch = epoch_;
        auto       observed_song  = source_song_;
        const bool playing        = state_.playback_state == i32();
        if (logged_song_.is_some() &&
            (logged_song_ != observed_song || logged_epoch_ != observed_epoch ||
             state_.playback_state == i32(2))) {
            auto id = logged_song_.take().unwrap();
            co_await log(id, PlayingAction::Stop);
        }
        if (playing && observed_song.is_some() && logged_song_.is_none()) {
            logged_song_  = observed_song;
            logged_epoch_ = observed_epoch;
            co_await log(*observed_song, PlayingAction::Play);
        }
        if (stopping_ || observed_epoch != epoch_ || generation != process_generation_)
            co_return empty {};
        if (event.kind == ipc::EventKind::Ended) {
            auto ticket = observed_intent;
            auto guard  = co_await commands_.lock();
            if (! stopping_ && ticket == intent_ && source_intent_ == ticket &&
                source_entry_id_ == queue_.current_entry_id() && observed_epoch == epoch_ &&
                desired_playing_) {
                auto next = co_await advance(false, true, ticket);
                if (next.is_err()) report_error(rstd::format("{}", next.unwrap_err()));
            }
        } else
            maybe_prefetch();
        co_return empty {};
    }
    static auto record_logs(sqlito::ExecutorHandle db, Rc<ProviderRuntime> providers,
                            SyncCoordinator coordinator, Receiver<PlaylogEntry> input)
        -> coro<empty> {
        while (auto entry = co_await input.recv()) {
            auto logged = co_await database_run(
                db.clone(),
                [entry       = *entry,
                 playing     = providers->playing_state(),
                 coordinator = coordinator.clone()](sqlito::Connection& connection) mutable {
                    return record_playlog(
                        connection, coordinator, *playing, entry.id, entry.timestamp, entry.action);
                });
            if (logged.is_err()) rstd::log::warn("cannot record playback: {}", logged.unwrap_err());
        }
        co_return empty {};
    }
    auto log(i64 id, PlayingAction action) -> coro<empty> {
        if (log_input_.is_some())
            (void)co_await log_input_->send(PlaylogEntry { id, Timestamp::now(), action });
        co_return empty {};
    }

public:
    PlayerSession(sqlito::ExecutorHandle db, Rc<ProviderRuntime> providers,
                  Arc<RadioSessions> radios, SyncCoordinator coordinator, PathBuf program,
                  String base, EventSink events, Option<PathBuf> preferences_file)
        : db_(rstd::move(db)),
          providers_(rstd::move(providers)),
          radios_(rstd::move(radios)),
          coordinator_(rstd::move(coordinator)),
          program_(rstd::move(program)),
          base_(rstd::move(base)),
          events_(rstd::move(events)),
          preferences_file_(rstd::move(preferences_file)) {}

    static auto make(sqlito::ExecutorHandle db, Rc<ProviderRuntime> providers,
                     Arc<RadioSessions> radios, SyncCoordinator coordinator, PathBuf program,
                     String base, EventSink events, Option<PathBuf> preferences_file = None())
        -> Arc<PlayerSession> {
        auto owner   = Arc<PlayerSession>::make(rstd::move(db),
                                                rstd::move(providers),
                                                rstd::move(radios),
                                                rstd::move(coordinator),
                                                rstd::move(program),
                                                rstd::move(base),
                                                rstd::move(events),
                                                rstd::move(preferences_file));
        owner->self_ = owner.downgrade();
        return owner;
    }
    auto state() const -> wire::M_PlayerStateMsg {
        wire::M_PlayerStateMsg message;
        message.f_revision       = state_revision_;
        message.f_source_epoch   = epoch_;
        message.f_connected      = connected_;
        message.f_source         = state_.source.clone();
        message.f_playback_state = state_.playback_state;
        message.f_position_ms    = state_.position_ms;
        message.f_duration_ms    = state_.duration_ms;
        message.f_volume         = f64(state_.volume);
        message.f_fade_ms        = state_.fade_ms;
        message.f_busy           = state_.busy;
        message.f_seekable       = state_.seekable;
        message.f_cache_begin    = f64(state_.cache_begin);
        message.f_cache_end      = f64(state_.cache_end);
        message.f_ended_serial   = ended_serial_;
        message.f_seek_serial    = seek_serial_;
        message.f_error          = error_.clone();
        if (auto id = source_song_) {
            auto field        = Box<p_qcm::p_msg::p_id::M_SongId>::make();
            field->f_value    = *id;
            message.f_song_id = Some(rstd::move(field));
            message.f_art_url = rstd::format("{}/image/Song/{}/Primary", base_, *id);
        }
        if (track_.is_some()) {
            message.f_title         = track_->title.clone();
            message.f_album         = track_->album.clone();
            message.f_artists       = track_->artists.clone();
            message.f_album_artists = track_->album_artists.clone();
        }
        return message;
    }
    auto queue_state() const -> wire::M_PlayerQueueMsg {
        wire::M_PlayerQueueMsg message;
        message.f_revision = queue_revision_;
        for (auto id : queue_.ids()) message.f_song_ids.push(rstd::move(id));
        for (auto id : queue_.entry_ids()) message.f_entry_ids.push(rstd::move(id));
        message.f_current_index    = queue_.current_index();
        message.f_current_entry_id = queue_.current_entry_id().unwrap_or(u64());
        message.f_loop_mode        = i32(static_cast<int>(queue_.loop_mode()));
        message.f_shuffle          = queue_.shuffle();
        message.f_random_mode      = queue_.random_mode();
        message.f_can_next         = queue_.can_next() || (radio_id_ > i64() && ! radio_exhausted_);
        message.f_can_previous     = queue_.can_previous();
        message.f_can_jump         = true;
        message.f_can_remove       = radio_id_ <= i64();
        message.f_name             = name_.clone();
        message.f_exhausted        = radio_exhausted_;
        message.f_configuration    = radio_configuration_.clone();
        if (radio_id_ > i64()) {
            auto id                     = Box<p_qcm::p_msg::p_id::M_RadioQueueId>::make();
            id->f_value                 = radio_id_;
            message.f_radio_queue_id    = Some(rstd::move(id));
            auto presentation           = Box<p_qcm::p_msg::p_model::M_QueuePresentation>::make();
            presentation->f_description = radio_presentation_.description.clone();
            presentation->f_source_item_id = radio_presentation_.source_item_id.is_some()
                                                 ? *radio_presentation_.source_item_id
                                                 : i64();
            message.f_presentation         = Some(rstd::move(presentation));
        }
        return message;
    }
    auto start() -> coro<empty> {
        if (preferences_file_.is_some()) {
            auto preferences = PlayerPreferences::load(preferences_file_->as_path());
            if (preferences.is_err()) {
                rstd::log::warn("cannot load playback preferences: {}", preferences.unwrap_err());
                preferences_file_ = None();
            } else {
                saved_preferences_ = configured_preferences_ = *preferences;
                state_.volume                                = preferences->volume;
                state_.fade_ms                               = preferences->fade_ms;
                (void)queue_.set_loop_mode(
                    static_cast<LoopMode>(preferences->loop_mode.to_primitive()));
                (void)queue_.set_shuffle(preferences->shuffle);
                (void)queue_.set_random_mode(preferences->random_mode);
            }
        }
        (void)co_await ensure_process();
        if (stopping_) co_return empty {};
        publisher_ = Some(AbortOnDropHandle<empty> {
            rstd::async::spawn_local(publish(self_.clone(), changed_.subscribe())) });
        changed(true);
        co_return empty {};
    }
    auto attach_integration(Box<dyn<PlayerIntegration>> integration)
        -> coro<PlayerIntegrationResult> {
        if (stopping_ || integration_.is_some()) {
            co_await integration->shutdown();
            co_return Err("player integration is unavailable or already attached"_Str);
        }
        integration_ = Some(rstd::move(integration));
        changed(true);
        co_return Ok(empty {});
    }
    auto control(wire::M_PlayerControlReq request) -> coro<ProcessResult<empty>>;
    auto queue(wire::M_PlayerQueueReq request) -> coro<ProcessResult<empty>>;
    auto shutdown() -> coro<empty> {
        stopping_        = true;
        desired_playing_ = false;
        ++intent_;
        save_preferences();
        if (prefetch_.is_some()) {
            prefetch_->abort();
            (void)co_await rstd::move(*prefetch_);
            prefetch_ = None();
        }
        if (publisher_.is_some()) {
            publisher_->abort();
            (void)co_await rstd::move(*publisher_);
            publisher_ = None();
        }
        if (auto integration = integration_.take()) co_await (*integration)->shutdown();
        if (auto process = process_.take()) co_await (*process)->shutdown();
        if (event_worker_.is_some()) {
            event_worker_->abort();
            (void)co_await rstd::move(*event_worker_);
            event_worker_ = None();
            event_input_  = None();
        }
        if (auto id = logged_song_.take()) co_await log(*id, PlayingAction::Stop);
        log_input_ = None();
        if (log_worker_.is_some()) {
            (void)co_await rstd::move(*log_worker_);
            log_worker_ = None();
        }
        co_await close_radio();
        for (auto& task : radio_closures_) (void)co_await rstd::move(task);
        radio_closures_.clear();
        co_return empty {};
    }
};

auto PlayerSession::control(wire::M_PlayerControlReq request) -> coro<ProcessResult<empty>> {
    using Action = wire::E_PlayerAction;
    auto action  = static_cast<Action>(request.f_action.to_primitive());
    if (stopping_) co_return Err(ProcessError::internal("player is stopping"_str));
    if (action == Action::v_PLAYER_ACTION_SOURCE && request.f_source.len() > usize(65536))
        co_return Err(ProcessError::internal("player source is too long"_str));
    if (action == Action::v_PLAYER_ACTION_PLAY || action == Action::v_PLAYER_ACTION_NEXT ||
        action == Action::v_PLAYER_ACTION_PREVIOUS)
        desired_playing_ = true;
    else if (action == Action::v_PLAYER_ACTION_PAUSE || action == Action::v_PLAYER_ACTION_STOP)
        desired_playing_ = false;
    else if (action == Action::v_PLAYER_ACTION_TOGGLE)
        desired_playing_ = ! desired_playing_;
    else if (action == Action::v_PLAYER_ACTION_SOURCE)
        desired_playing_ = ! request.f_source.is_empty();
    auto disruptive =
        action == Action::v_PLAYER_ACTION_SOURCE || action == Action::v_PLAYER_ACTION_STOP ||
        action == Action::v_PLAYER_ACTION_NEXT || action == Action::v_PLAYER_ACTION_PREVIOUS;
    auto ticket = disruptive ? ++intent_ : intent_;
    auto guard  = co_await commands_.lock();
    if (stopping_) co_return Err(ProcessError::internal("player is stopping"_str));
    if (ticket != intent_) co_return Ok(empty {});
    switch (action) {
    case Action::v_PLAYER_ACTION_PLAY:
    case Action::v_PLAYER_ACTION_TOGGLE: {
        if (! desired_playing_) co_return co_await issue(ticket, ipc::Action::Pause);
        if (queue_.current_id().is_none() && ! queue_.ids().is_empty()) {
            (void)rstd_co_try(queue_.select(i32()), ProcessError::internal(rstd::move(_e)));
            changed(true);
        }
        if (queue_.current_id().is_some() && (state_.source.is_empty() || process_failed_ ||
                                              source_entry_id_ != queue_.current_entry_id()))
            co_return co_await select_source(ticket);
        if (state_.source.is_empty()) co_return Ok(empty {});
        co_return co_await issue(ticket, ipc::Action::Play);
    }
    case Action::v_PLAYER_ACTION_PAUSE: co_return co_await issue(ticket, ipc::Action::Pause);
    case Action::v_PLAYER_ACTION_STOP: co_return co_await issue(ticket, ipc::Action::Stop);
    case Action::v_PLAYER_ACTION_SEEK: {
        if (request.f_expected_source_epoch != u64() && request.f_expected_source_epoch != epoch_)
            co_return Ok(empty {});
        if (! state_.seekable) co_return Ok(empty {});
        auto position = request.f_position_ms.max(i64());
        if (position > i64(i32::MAX.to_primitive()))
            co_return Err(ProcessError::internal("seek position exceeds player range"_str));
        if (state_.duration_ms > i64()) position = position.min(state_.duration_ms);
        co_return co_await issue(
            ticket, ipc::Action::Seek, static_cast<double>(position.to_primitive()));
    }
    case Action::v_PLAYER_ACTION_VOLUME:
        if (! f64(request.f_volume).is_finite())
            co_return Err(ProcessError::internal("volume must be finite"_str));
        co_return co_await issue(
            ticket, ipc::Action::Volume, request.f_volume.max(f64()).min(f64(1)).to_primitive());
    case Action::v_PLAYER_ACTION_FADE:
        if (request.f_fade_ms > u32::MAX / u32(1000))
            co_return Err(ProcessError::internal("fade duration exceeds player range"_str));
        co_return co_await issue(
            ticket, ipc::Action::Fade, static_cast<double>(request.f_fade_ms.to_primitive()));
    case Action::v_PLAYER_ACTION_NEXT: co_return co_await advance(false, false, ticket);
    case Action::v_PLAYER_ACTION_PREVIOUS: co_return co_await advance(true, false, ticket);
    case Action::v_PLAYER_ACTION_SOURCE:
        if (request.f_source.len() > usize(65536))
            co_return Err(ProcessError::internal("player source is too long"_str));
        co_await close_radio();
        (void)rstd_co_try(queue_.clear(), ProcessError::internal(rstd::move(_e)));
        track_ = None();
        changed(true);
        co_return co_await issue(ticket, ipc::Action::Source, 0, rstd::move(request.f_source));
    case Action::v_PLAYER_ACTION_SNAPSHOT: changed(true); co_return Ok(empty {});
    case Action::v_PLAYER_ACTION_UNSPECIFIED: break;
    }
    co_return Err(ProcessError::internal("unsupported player action"_str));
}

auto PlayerSession::queue(wire::M_PlayerQueueReq request) -> coro<ProcessResult<empty>> {
    using Action = wire::E_PlayerQueueAction;
    auto action  = static_cast<Action>(request.f_action.to_primitive());
    if (stopping_) co_return Err(ProcessError::internal("player is stopping"_str));
    if (action == Action::v_PLAYER_QUEUE_ACTION_REPLACE &&
        (request.f_index < i32(-1) ||
         (request.f_index >= i32() &&
          usize(static_cast<rstd::size_t>(request.f_index.to_primitive())) >=
              request.f_song_ids.len())))
        co_return Err(ProcessError::internal("player queue replacement index out of range"_str));
    if (request.f_play) desired_playing_ = true;
    if (action == Action::v_PLAYER_QUEUE_ACTION_CLEAR) desired_playing_ = false;
    bool disruptive = action == Action::v_PLAYER_QUEUE_ACTION_REPLACE ||
                      action == Action::v_PLAYER_QUEUE_ACTION_SELECT ||
                      action == Action::v_PLAYER_QUEUE_ACTION_CLEAR ||
                      action == Action::v_PLAYER_QUEUE_ACTION_RADIO || request.f_play;
    auto ticket     = disruptive ? ++intent_ : intent_;
    auto guard      = co_await commands_.lock();
    if (stopping_) co_return Err(ProcessError::internal("player is stopping"_str));
    if (ticket != intent_) co_return Ok(empty {});
    if (request.f_expected_revision != u64() && request.f_expected_revision != queue_revision_)
        co_return Err(ProcessError::internal("playback queue changed; refresh before editing"_str));
    for (auto id : request.f_song_ids) {
        if (id <= i64()) co_return Err(ProcessError::WrongId(rstd::format("{}", id)));
    }
    const bool grows = action == Action::v_PLAYER_QUEUE_ACTION_APPEND ||
                       action == Action::v_PLAYER_QUEUE_ACTION_INSERT_NEXT;
    if (request.f_song_ids.len() > usize(100000) ||
        (grows && queue_.ids().len() > usize(100000) - request.f_song_ids.len()))
        co_return Err(ProcessError::internal("playback queue is too large"_str));
    PlayerQueueChange change = PlayerQueueChange::Unchanged;
    switch (action) {
    case Action::v_PLAYER_QUEUE_ACTION_REPLACE:
        co_await close_radio();
        name_  = rstd::move(request.f_name);
        change = rstd_co_try(queue_.replace(rstd::move(request.f_song_ids), request.f_index),
                             ProcessError::internal(rstd::move(_e)));
        break;
    case Action::v_PLAYER_QUEUE_ACTION_APPEND: {
        auto first = i32(static_cast<int>(queue_.ids().len().to_primitive()));
        change     = rstd_co_try(queue_.append(rstd::move(request.f_song_ids)),
                                 ProcessError::internal(rstd::move(_e)));
        if (request.f_play &&
            usize(static_cast<rstd::size_t>(first.to_primitive())) < queue_.ids().len())
            change = rstd_co_try(queue_.select(first), ProcessError::internal(rstd::move(_e)));
        break;
    }
    case Action::v_PLAYER_QUEUE_ACTION_INSERT_NEXT:
        change = rstd_co_try(queue_.insert_next(rstd::move(request.f_song_ids)),
                             ProcessError::internal(rstd::move(_e)));
        break;
    case Action::v_PLAYER_QUEUE_ACTION_SELECT:
        change =
            rstd_co_try(queue_.select(request.f_index), ProcessError::internal(rstd::move(_e)));
        break;
    case Action::v_PLAYER_QUEUE_ACTION_REMOVE:
        if (radio_id_ > i64())
            co_return Err(ProcessError::internal("radio entries cannot be removed"_str));
        change = rstd_co_try(queue_.remove(request.f_index, request.f_count),
                             ProcessError::internal(rstd::move(_e)));
        break;
    case Action::v_PLAYER_QUEUE_ACTION_MOVE:
        change = rstd_co_try(queue_.move(request.f_index, request.f_destination, request.f_count),
                             ProcessError::internal(rstd::move(_e)));
        break;
    case Action::v_PLAYER_QUEUE_ACTION_CLEAR:
        co_await close_radio();
        name_.clear();
        change = rstd_co_try(queue_.clear(), ProcessError::internal(rstd::move(_e)));
        break;
    case Action::v_PLAYER_QUEUE_ACTION_LOOP: {
        auto mode = request.f_loop_mode.to_primitive();
        if (mode < 0 || mode > 2) co_return Err(ProcessError::internal("invalid loop mode"_str));
        change = rstd_co_try(queue_.set_loop_mode(static_cast<LoopMode>(mode)),
                             ProcessError::internal(rstd::move(_e)));
        if (request.f_shuffle.is_some())
            (void)rstd_co_try(queue_.set_shuffle(*request.f_shuffle),
                              ProcessError::internal(rstd::move(_e)));
        configured_preferences_.loop_mode = request.f_loop_mode;
        configured_preferences_.shuffle   = queue_.shuffle();
        break;
    }
    case Action::v_PLAYER_QUEUE_ACTION_SHUFFLE:
        if (request.f_shuffle.is_none())
            co_return Err(ProcessError::internal("shuffle value is required"_str));
        change                          = rstd_co_try(queue_.set_shuffle(*request.f_shuffle),
                                                      ProcessError::internal(rstd::move(_e)));
        configured_preferences_.shuffle = *request.f_shuffle;
        break;
    case Action::v_PLAYER_QUEUE_ACTION_RANDOM:
        change = rstd_co_try(queue_.set_random_mode(request.f_random_mode),
                             ProcessError::internal(rstd::move(_e)));
        configured_preferences_.random_mode = request.f_random_mode;
        break;
    case Action::v_PLAYER_QUEUE_ACTION_RADIO: {
        if (request.f_radio_queue_id.is_none() || (*request.f_radio_queue_id)->f_value <= i64())
            co_return Err(ProcessError::internal("radio queue ID is required"_str));
        wire::M_OpenQueueReq open;
        open.f_queue_id      = rstd::move(request.f_radio_queue_id);
        open.f_configuration = rstd::move(request.f_configuration);
        open.f_reason        = request.f_radio_reason;
        if ((*open.f_queue_id)->f_value == radio_id_)
            open.f_previous_session_id = radio_session_.clone();
        open.f_operation_id = rstd::format("{}:{}", current_millis(), new_instance_id());
        auto  opened        = rstd_co_try(co_await open_queue_async(
            *providers_.get(), *radios_, coordinator_, db_.clone(), i32(), open));
        auto& page          = *opened.o_payload->as_open_queue_rsp();
        if (page.f_session.is_none() || page.f_song_ids.is_none())
            co_return Err(ProcessError::internal("radio returned an incomplete session"_str));
        if (ticket != intent_ || stopping_) {
            co_await radios_->close(
                *providers_.get(), coordinator_, db_.clone(), (*page.f_session)->f_id.clone());
            co_return Ok(empty {});
        }
        co_await close_radio();
        radio_id_ = (*open.f_queue_id)->f_value;
        adopt_radio(**page.f_session);
        name_ = rstd::move(request.f_name);
        (void)rstd_co_try(queue_.set_shuffle(false), ProcessError::internal(rstd::move(_e)));
        (void)rstd_co_try(queue_.set_loop_mode(LoopMode::None),
                          ProcessError::internal(rstd::move(_e)));
        auto index = (*page.f_song_ids)->f_values.is_empty() ? i32(-1) : i32();
        change     = rstd_co_try(queue_.replace(rstd::move((*page.f_song_ids)->f_values), index),
                                 ProcessError::internal(rstd::move(_e)));
        break;
    }
    case Action::v_PLAYER_QUEUE_ACTION_UNSPECIFIED:
    default: co_return Err(ProcessError::internal("unsupported player queue action"_str));
    }
    changed(true);
    if (request.f_play && queue_.current_id().is_none() && ! queue_.ids().is_empty())
        change = rstd_co_try(queue_.select(i32()), ProcessError::internal(rstd::move(_e)));
    if (queue_.current_id().is_none() && (action == Action::v_PLAYER_QUEUE_ACTION_REPLACE ||
                                          action == Action::v_PLAYER_QUEUE_ACTION_CLEAR ||
                                          action == Action::v_PLAYER_QUEUE_ACTION_RADIO)) {
        rstd_co_try(co_await clear_source(ticket));
    } else if (change == PlayerQueueChange::CurrentChanged ||
               change == PlayerQueueChange::Restart) {
        if (queue_.current_id().is_none())
            rstd_co_try(co_await clear_source(ticket));
        else
            rstd_co_try(co_await select_source(ticket));
    } else if (request.f_play && queue_.current_id().is_some()) {
        if (state_.source.is_empty() || process_failed_ ||
            source_entry_id_ != queue_.current_entry_id())
            rstd_co_try(co_await select_source(ticket));
        else
            rstd_co_try(co_await issue(ticket, ipc::Action::Play));
    }
    if (action == Action::v_PLAYER_QUEUE_ACTION_RADIO && queue_.ids().is_empty() &&
        ! radio_exhausted_ && desired_playing_) {
        pending_advance_ = Some(PendingAdvance { ticket, epoch_, false });
        maybe_prefetch(true);
    } else
        maybe_prefetch();
    co_return Ok(empty {});
}
} // namespace qcm::backend
