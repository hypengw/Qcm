module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:api.provider;
export import :protocol.message;
export import :protocol.provider;
export import :error;
export import rstd;
import :provider.registry;
import :provider.runtime;
import :provider.item;
import :model.item;
import :db.provider.store;
import :db.provider.favorite;
import :db.query.subtitle;
import :db.query.radio;
import :db.sync.run;
import :db.database;
import sqlito;
import lihttpto;
import rstd.json;
import :identity;
template<class T>
using AsyncMutex = rstd::async::Mutex<T>;
using alloc::rc::make_rc;
using alloc::rc::Rc;
using rstd::tuple;
using rstd::collections::BTreeMap;
using rstd::sync::Arc;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
auto get_provider_metas(const ProviderRegistry& registry, i32 request_id)
    -> p_qcm::p_msg::M_QcmMessage {
    auto rsp = Box<p_qcm::p_msg::M_GetProviderMetasRsp>::make();
    for (const auto& meta : registry.metas())
        rsp->f_metas.push(rstd::into<Box<p_qcm::p_msg::p_model::M_ProviderMeta>>(
            ref<ProviderMetaInfo>::from_raw_parts(rstd::addressof(meta))));
    return qcm_message(request_id, rstd::move(rsp));
}

struct TmpProviderEntry {
    String                 key;
    String                 type_name;
    ProviderHandle         instance;
    bool                   promoted {};
    Arc<AsyncMutex<empty>> gate { Arc<AsyncMutex<empty>>::make(empty {}) };
};
using TmpProviderHandle = Rc<TmpProviderEntry>;

class TmpProviderStore {
    BTreeMap<String, TmpProviderHandle> entries_;

public:
    auto find(ref<str> key) -> TmpProviderEntry* {
        auto entry = entries_.get_mut(key);
        return entry.is_some() ? (*entry)->get() : nullptr;
    }
    auto acquire(ref<str> key) -> TmpProviderHandle {
        auto entry = entries_.get(key);
        return entry.is_some() ? (*entry)->clone() : TmpProviderHandle {};
    }
    auto erase(ref<str> key) -> bool {
        // In-flight requests retain ownership after the lookup entry is removed.
        return entries_.remove(key).is_some();
    }
    auto push(TmpProviderEntry entry) -> void {
        auto key = entry.key.clone();
        entries_.insert(rstd::move(key), make_rc<TmpProviderEntry>(rstd::move(entry)));
    }
    auto len() const -> usize { return entries_.len(); }
    auto close_async() -> rstd::async::coro<empty> {
        Vec<ProviderHandle> providers;
        for (auto entry : entries_.values())
            if (! entry->get()->promoted) providers.push(entry->get()->instance.clone());
        for (auto& provider : providers) (void)co_await provider.get()->close_async();
        co_return empty {};
    }
};

enum class ProviderMutationKind
{
    Added,
    Updated,
    Replaced,
    Deleted
};
class ProviderLiveMutation {
    RSTD_ENUM(
        ProviderLiveMutation,
        (Add, (ProviderMetaInfo meta; TmpProviderHandle temporary; String name; String key;)),
        (Update, (ProviderHandle instance; String name; SettingsSnapshot settings;)),
        (Replace, (
            ProviderHandle previous;
            ProviderHandle candidate;
            TmpProviderHandle temporary;
            String name;
            String key;
            SettingsSnapshot settings;
        )),
        (Delete)
    )
};
struct ProviderCommittedMutation {
    ProviderMutationKind kind;
    i64                  provider_id;
    ProviderLiveMutation live;

    auto install(ProviderRuntime& runtime, sqlito::ExecutorHandle db) -> ProviderHandle {
        RSTD_MATCH(live) {
            RSTD_CASE(Add, meta, temporary, name, key) {
                auto candidate = temporary->instance.clone();
                if (candidate.get()->set_identity(provider_id, name.as_str()).is_err())
                    rstd::log::error("provider {} committed but could not update its identity",
                                     provider_id);
                candidate.get()->set_session_store(rstd::move(db), provider_id, rstd::move(key));
                runtime.add(provider_id, meta, rstd::move(candidate));
                temporary.get()->promoted = true;
            }
            RSTD_CASE(Update, instance, name, settings) {
                auto renamed = instance.get()->set_name(name.as_str());
                auto applied = instance.get()->set_settings(rstd::move(settings));
                if (renamed.is_err() || applied.is_err())
                    rstd::log::error("provider {} committed but could not update its live state",
                                     provider_id);
            }
            RSTD_CASE(Replace, previous, candidate, temporary, name, key, settings) {
                if (candidate.get()->set_settings(rstd::move(settings)).is_err())
                    rstd::log::error("provider {} committed but could not update its settings",
                                     provider_id);
                if (candidate.get()->set_identity(provider_id, name.as_str()).is_err())
                    rstd::log::error("provider {} committed but could not update its identity",
                                     provider_id);
                candidate.get()->set_session_store(rstd::move(db), provider_id, rstd::move(key));
                (void)runtime.replace(provider_id, rstd::move(candidate));
                if (temporary) temporary.get()->promoted = true;
                return rstd::move(previous);
            }
            RSTD_CASE(Delete) {}
        }
        return {};
    }
    auto replaces_instance() const -> bool { return live.is_Replace() || live.is_Delete(); }
};
class ProviderMutationOutcome {
    RSTD_ENUM(
        ProviderMutationOutcome,
        (Committed, (ProviderCommittedMutation mutation;)),
        (SettingsRejected, (SettingsError error;)),
        (AuthenticationRejected, (AuthOutcome outcome;))
    )
};

class ProviderMutationWrite {
    ProviderCancelHandle cancellable_;

public:
    explicit ProviderMutationWrite(ProviderCancelHandle cancellable = {})
        : cancellable_(rstd::move(cancellable)) {}
    template<class F>
    auto run(sqlito::ExecutorHandle db, F function)
        -> rstd::async::coro<decltype(function(rstd::declval<sqlito::Connection&>()))> {
        // The worker may commit before its result wakes this task. Seal cancellation before submit.
        if (cancellable_) *cancellable_.get() = false;
        co_return co_await database_run(rstd::move(db), rstd::move(function));
    }
};

class ProviderAuthenticationUpdate {
    RSTD_ENUM(
        ProviderAuthenticationUpdate,
        (Unchanged),
        (Authenticate, (AuthInput input;)),
        (Conflict)
    )
};
struct ProviderUpdateRequest {
    i64                          provider_id;
    String                       name;
    String                       temporary;
    String                       settings_json;
    u64                          settings_revision;
    ProviderAuthenticationUpdate authentication;
};

auto json_escape(ref<str> value) -> String {
    auto encoded = rstd::json::to_string(rstd::json::Value::String(String::make(value)));
    return String::make(encoded.as_str().get(usize(1), encoded.len() - usize(1)).unwrap());
}
auto encode_provider_auth(const Provider& provider) -> ProcessResult<String> {
    auto method = provider.auth_method();
    if (method.is_none()) return Ok(String::make());
    return rstd::json::encode(*method).map_err([](auto error) {
        return ProcessError::from_provider(ProviderError::serde(rstd::move(error)));
    });
}
auto provider_ok(i32 request_id) -> p_qcm::p_msg::M_QcmMessage {
    auto rsp    = Box<p_qcm::p_msg::M_Rsp>::make();
    rsp->f_code = i32();
    return qcm_message(request_id, rstd::move(rsp));
}
auto create_tmp_provider(TmpProviderStore& store, const ProviderRegistry& registry,
                         ProviderRuntime& runtime, i32 request_id,
                         const p_qcm::p_msg::M_CreateTmpProviderReq& entry)
    -> Result<p_qcm::p_msg::M_QcmMessage, ProcessError> {
    const auto* meta = registry.find(entry.f_type_name.as_str());
    if (meta == nullptr) return Err(ProcessError::NoSuchProviderType(entry.f_type_name.clone()));
    ProviderOptions options;
    options.type_name = meta->type_name.clone();
    options.name      = String::make(""_str);
    options.base_url  = String::make(""_str);
    auto instance     = rstd_try(runtime.create(registry, rstd::move(options)));
    auto key          = new_instance_id();
    store.push(TmpProviderEntry { key.clone(),
                                  meta->type_name.clone(),
                                  rstd::move(instance),
                                  false,
                                  Arc<AsyncMutex<empty>>::make(empty {}) });
    auto rsp   = Box<p_qcm::p_msg::M_CreateTmpProviderRsp>::make();
    rsp->f_key = rstd::move(key);
    return Ok(qcm_message(request_id, rstd::move(rsp)));
}
auto delete_tmp_provider(TmpProviderStore& store, i32 request_id,
                         const p_qcm::p_msg::M_DeleteTmpProviderReq& entry)
    -> Result<p_qcm::p_msg::M_QcmMessage, ProcessError> {
    (void)store.erase(entry.f_key.as_str());
    return Ok(provider_ok(request_id));
}
auto auth_response(i32 request_id, AuthOutcome outcome) -> p_qcm::p_msg::M_QcmMessage {
    return qcm_message(request_id,
                       rstd::into<Box<p_qcm::p_msg::M_AuthProviderRsp>>(rstd::move(outcome)));
}
auto auth_provider_async(TmpProviderStore& store, i32 request_id,
                         const p_qcm::p_msg::M_AuthProviderReq& entry)
    -> rstd::async::coro<Result<p_qcm::p_msg::M_QcmMessage, ProcessError>> {
    auto target = store.acquire(entry.f_tmp_provider.as_str());
    if (! target) co_return Err(ProcessError::NoSuchProvider(entry.f_tmp_provider.clone()));
    if (entry.f_auth_info.is_none())
        co_return Err(ProcessError::MissingFields(String::make("auth_info"_str)));
    auto input = rstd::into<AuthInput>((*entry.f_auth_info).as_ref());
    auto outcome =
        rstd_co_try(co_await target.get()->instance.get()->auth_async(rstd::move(input)));
    co_return Ok(auth_response(request_id, rstd::move(outcome)));
}
auto add_provider_async(TmpProviderStore& store, const ProviderRegistry& registry,
                        sqlito::ExecutorHandle db, const p_qcm::p_msg::M_AddProviderReq& entry,
                        ProviderMutationWrite& write)
    -> rstd::async::coro<ProcessResult<ProviderMutationOutcome>> {
    auto target = store.acquire(entry.f_tmp_provider.as_str());
    if (! target) co_return Err(ProcessError::NoSuchProvider(entry.f_tmp_provider.clone()));
    const auto* meta = registry.find(target->type_name.as_str());
    if (meta == nullptr) co_return Err(ProcessError::NoSuchProviderType(target->type_name.clone()));
    auto           custom      = rstd_co_try(target.get()->instance.get()->save_session());
    auto           cookie      = rstd_co_try(target.get()->instance.get()->save_cookie());
    auto           session_key = new_instance_id();
    auto           name        = entry.f_name.clone();
    ProviderCreate input { name.clone(),
                           target->type_name.clone(),
                           ProviderSessionWrite {
                               target->instance->base_url(),
                               rstd_co_try(encode_provider_auth(*target->instance)),
                               rstd::move(custom),
                               rstd::move(cookie),
                               session_key.clone() } };
    auto           id = rstd_co_try(co_await write.run(
        db.clone(), [input = rstd::move(input)](sqlito::Connection& connection) mutable {
            return insert_provider(connection, rstd::move(input));
        }));
    co_return Ok(ProviderMutationOutcome::Committed(ProviderCommittedMutation {
        ProviderMutationKind::Added,
        id,
        ProviderLiveMutation::Add(
            meta->clone(), rstd::move(target), rstd::move(name), rstd::move(session_key)) }));
}
auto get_provider_settings(ProviderRuntime& runtime, i32 request_id,
                           const p_qcm::p_msg::M_GetProviderSettingsReq& entry)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage> {
    auto provider_id = rstd_try(required_id(entry.f_provider_id, "provider_id"_str));
    auto instance    = runtime.acquire(provider_id);
    if (! instance) return Err(ProcessError::NoSuchProvider(rstd::format("{}", provider_id)));
    auto              snapshot = instance->settings();
    rstd::json::Array definitions;
    for (const auto& field : instance->settings_definition())
        definitions.push(
            rstd_try(rstd::json::to_value(field), ProcessError::internal(rstd::move(_e))));
    auto definition = rstd::json::to_string(rstd::json::Value::Array(rstd::move(definitions)));
    auto rsp        = Box<p_qcm::p_msg::M_GetProviderSettingsRsp>::make();
    rsp->f_definition_json = rstd::move(definition);
    rsp->f_values_json =
        rstd_try(rstd::json::encode(snapshot.values), ProcessError::internal(rstd::move(_e)));
    rsp->f_revision = snapshot.revision;
    return Ok(qcm_message(request_id, rstd::move(rsp)));
}

struct SubtitlePlan {
    i64            song_id;
    SubtitleTarget target;
    ProviderHandle instance;
    u64            epoch;
};
auto prepare_subtitle(ProviderRuntime& runtime, sqlito::ExecutorHandle db,
                      const p_qcm::p_msg::M_GetSubtitleReq& entry)
    -> rstd::async::coro<Result<SubtitlePlan, ProcessError>> {
    auto song_id = rstd_co_try(required_id(entry.f_song_id, "song_id"_str));
    auto target  = rstd_co_try(
        co_await database_run(rstd::move(db), [song_id](sqlito::Connection& connection) {
            return subtitle_target(connection, song_id);
        }));
    if (target.is_none()) co_return Err(ProcessError::NoSuchSong(rstd::format("{}", song_id)));
    auto instance = runtime.acquire(target->provider_id);
    if (! instance)
        co_return Err(ProcessError::NoSuchProvider(rstd::format("{}", target->provider_id)));
    auto epoch = instance->session_epoch();
    if (epoch.is_none()) co_return Err(ProcessError::NotAuth());
    co_return Ok(SubtitlePlan { song_id, rstd::move(*target), rstd::move(instance), *epoch });
}
auto execute_subtitle(ProviderRuntime& runtime, sqlito::ExecutorHandle db, i32 request_id,
                      SubtitlePlan plan)
    -> rstd::async::coro<Result<p_qcm::p_msg::M_QcmMessage, ProcessError>> {
    auto call = runtime.call_snapshot(plan.target.provider_id, plan.instance, Some(plan.epoch));
    if (call.is_none()) co_return Err(ProcessError::NotAuth());
    auto valid = [snapshot = call->clone()] {
        return snapshot.is_current();
    };
    auto subtitle = co_await plan.instance.get()->subtitle_async(plan.target.native_id.clone());
    if (! valid()) co_return Err(ProcessError::NotAuth());
    auto loaded_subtitle = rstd_co_try(rstd::move(subtitle));
    auto current = co_await database_run(rstd::move(db),
                                         [song_id = plan.song_id](sqlito::Connection& connection) {
                                             return subtitle_target(connection, song_id);
                                         });
    if (! valid()) co_return Err(ProcessError::NotAuth());
    auto current_target = rstd_co_try(rstd::move(current));
    if (current_target.is_none() || current_target->provider_id != plan.target.provider_id ||
        current_target->native_id != plan.target.native_id)
        co_return Err(ProcessError::NotAuth());
    auto rsp = Box<p_qcm::p_msg::M_GetSubtitleRsp>::make();
    rsp->f_subtitle =
        Some(rstd::into<Box<p_qcm::p_msg::p_model::M_Subtitle>>(rstd::move(loaded_subtitle)));
    co_return Ok(qcm_message(request_id, rstd::move(rsp)));
}
struct FavoritePlan {
    ProviderItem   target;
    ProviderHandle instance;
    u64            epoch;
};
auto prepare_favorite(ProviderRuntime& runtime, sqlito::ExecutorHandle db,
                      const p_qcm::p_msg::M_SetFavoriteReq& entry)
    -> rstd::async::coro<Result<FavoritePlan, ProcessError>> {
    auto resolved = rstd_co_try(co_await resolve_provider_item(
        rstd::move(db), runtime, entry.f_id, i64(entry.f_item_type.to_primitive())));
    if (resolved.is_none())
        co_return Err(ProcessError::UnsupportedItemType(rstd::format("{}", entry.f_item_type)));
    auto instance = runtime.acquire(resolved->provider_id);
    if (! instance)
        co_return Err(ProcessError::NoSuchProvider(rstd::format("{}", resolved->provider_id)));
    auto epoch = instance->session_epoch();
    if (epoch.is_none()) co_return Err(ProcessError::NotAuth());
    co_return Ok(FavoritePlan { rstd::move(*resolved), rstd::move(instance), *epoch });
}
auto execute_favorite(ProviderRuntime& runtime, SyncCoordinator& coordinator,
                      sqlito::ExecutorHandle db, i32 request_id,
                      const p_qcm::p_msg::M_SetFavoriteReq& entry, FavoritePlan plan)
    -> rstd::async::coro<Result<p_qcm::p_msg::M_QcmMessage, ProcessError>> {
    auto call = runtime.call_snapshot(plan.target.provider_id, plan.instance, Some(plan.epoch));
    if (call.is_none()) co_return Err(ProcessError::NotAuth());
    auto valid = [snapshot = call->clone()] {
        return snapshot.is_current();
    };
    auto key = plan.instance->session_key();
    if (key.is_none()) co_return Err(ProcessError::NotAuth());
    auto context = call->context(db.clone(), coordinator.clone());
    auto called  = co_await plan.instance.get()->favorite_async(
        rstd::move(context), plan.target.native_id.clone(), plan.target.type, entry.f_value);
    if (! valid()) co_return Err(ProcessError::NotAuth());
    rstd_co_try(rstd::move(called));
    ProviderFavoriteWrite input { ProviderItemRead { entry.f_id, plan.target.type },
                                  ProviderItem { plan.target.id,
                                                 plan.target.provider_id,
                                                 plan.target.native_id.clone(),
                                                 plan.target.type },
                                  rstd::move(*key),
                                  entry.f_value };
    auto                  written = co_await database_run(
        rstd::move(db),
        [coordinator = coordinator.clone(),
         input       = rstd::move(input)](sqlito::Connection& connection) mutable {
            return write_provider_favorite(connection, coordinator, rstd::move(input));
        });
    if (! valid()) co_return Err(ProcessError::NotAuth());
    rstd_co_try(rstd::move(written));
    co_return Ok(provider_ok(request_id));
}
auto update_auth_response(i32 request_id, AuthOutcome outcome) -> p_qcm::p_msg::M_QcmMessage {
    return qcm_message(request_id,
                       rstd::into<Box<p_qcm::p_msg::M_UpdateProviderRsp>>(rstd::move(outcome)));
}
auto update_settings_response(i32 request_id, SettingsError error) -> p_qcm::p_msg::M_QcmMessage {
    auto rsp       = Box<p_qcm::p_msg::M_UpdateProviderRsp>::make();
    rsp->f_code    = i32(2);
    rsp->f_message = rstd::move(error.message);
    for (auto field : error.fields.iter())
        rsp->f_field_errors.insert(rstd::get<0>(field)->clone(), rstd::get<1>(field)->clone());
    return qcm_message(request_id, rstd::move(rsp));
}
auto normalize_provider_settings(Provider& candidate, ref<str> settings_json)
    -> Result<SettingsValues, SettingsError> {
    auto values = rstd::json::decode<SettingsValues>(settings_json);
    if (values.is_err()) {
        SettingsError error;
        error.message = "Settings must be a JSON object"_Str;
        error.cause   = Some(Arc<ErrorCause>::make(error_cause(rstd::move(values).unwrap_err())));
        return Err(rstd::move(error));
    }
    return candidate.validate_settings(*values);
}
auto update_provider_async(TmpProviderStore& store, ProviderRuntime& runtime,
                           SyncCoordinator& coordinator, const ProviderRegistry& registry,
                           sqlito::ExecutorHandle db, ProviderUpdateRequest entry,
                           ProviderMutationWrite& write)
    -> rstd::async::coro<ProcessResult<ProviderMutationOutcome>> {
    auto provider_id = entry.provider_id;
    auto row         = rstd_co_try(
        co_await database_run(db.clone(), [provider_id](sqlito::Connection& connection) {
            return read_provider_identity(connection, provider_id);
        }));
    if (entry.authentication.is_Conflict()) {
        SettingsError error;
        error.message = "Choose one authentication update"_Str;
        co_return Ok(ProviderMutationOutcome::SettingsRejected(rstd::move(error)));
    }
    auto old = runtime.acquire(provider_id);
    if (! old) co_return Err(ProcessError::NoSuchProvider(rstd::format("{}", provider_id)));
    if (! old->matches_session(provider_id, row.session_key.as_str()))
        co_return Err(ProcessError::NotAuth());
    auto temporary = store.acquire(entry.temporary.as_str());
    if (! entry.temporary.is_empty() && ! temporary)
        co_return Err(ProcessError::NoSuchProvider(entry.temporary.clone()));
    if (temporary && temporary->type_name != row.type_name) co_return Err(ProcessError::NotAuth());
    ProviderHandle fresh;
    if (entry.authentication.is_Authenticate()) {
        const auto* meta = registry.find(row.type_name.as_str());
        if (meta == nullptr) co_return Err(ProcessError::NoSuchProviderType(row.type_name.clone()));
        ProviderOptions options;
        options.type_name = meta->type_name.clone();
        options.name      = entry.name.clone();
        options.id        = Some(provider_id);
        fresh             = rstd_co_try(runtime.create(registry, rstd::move(options)));
    }
    auto* candidate = temporary ? temporary.get()->instance.get() : fresh ? fresh.get() : old.get();
    auto  settings  = normalize_provider_settings(*candidate, entry.settings_json.as_str());
    if (settings.is_err())
        co_return Ok(ProviderMutationOutcome::SettingsRejected(rstd::move(settings).unwrap_err()));
    auto settings_values = rstd::move(*settings);
    if (candidate != old.get())
        rstd_co_try(candidate->set_settings(
            SettingsSnapshot { entry.settings_revision, settings_values.clone() }));
    rstd::log::info("update provider {} tmp={} auth={}",
                    provider_id,
                    entry.temporary.is_empty() ? 0 : 1,
                    entry.authentication.is_Authenticate() ? 1 : 0);
    String name           = entry.name.clone();
    bool   replace_fields = false;
    String custom;
    String base_url;
    String auth_json;
    String cookie;
    if (temporary && temporary->instance.get() != old.get()) {
        custom         = rstd_co_try(temporary.get()->instance.get()->save_session());
        base_url       = temporary->instance->base_url();
        auth_json      = rstd_co_try(encode_provider_auth(*temporary->instance));
        cookie         = rstd_co_try(temporary.get()->instance.get()->save_cookie());
        replace_fields = true;
    } else if (entry.authentication.is_Authenticate()) {
        auto& authentication = entry.authentication.as_Authenticate().input;
        auto  outcome = rstd_co_try(co_await fresh.get()->auth_async(authentication.clone()));
        rstd::log::info("provider {} login succeeded {}", provider_id, outcome.is_Ok());
        if (! outcome.is_Ok())
            co_return Ok(ProviderMutationOutcome::AuthenticationRejected(rstd::move(outcome)));
        if (runtime.find(provider_id) != old.get()) co_return Err(ProcessError::NotAuth());
        custom         = rstd_co_try(fresh.get()->save_session());
        base_url       = fresh->base_url();
        auth_json      = rstd_co_try(encode_provider_auth(*fresh));
        cookie         = rstd_co_try(fresh.get()->save_cookie());
        replace_fields = true;
    }
    if (runtime.find(provider_id) != old.get()) co_return Err(ProcessError::NotAuth());
    auto                         new_key = replace_fields ? new_instance_id() : String::make();
    Option<ProviderSessionWrite> replacement;
    if (replace_fields)
        replacement = Some(ProviderSessionWrite { rstd::move(base_url),
                                                  rstd::move(auth_json),
                                                  rstd::move(custom),
                                                  rstd::move(cookie),
                                                  new_key.clone() });
    ProviderUpdate input_update { provider_id,
                                  name.clone(),
                                  row.session_key.clone(),
                                  rstd::move(replacement),
                                  ProviderSettingsWrite {
                                      rstd_co_try(rstd::json::encode(settings_values),
                                                  ProcessError::internal(rstd::move(_e))),
                                      entry.settings_revision } };
    auto           committed = rstd_co_try(co_await write.run(
        db.clone(),
        [coordinator = coordinator.clone(),
         input       = rstd::move(input_update)](sqlito::Connection& connection) mutable {
            return update_provider_record(connection, coordinator, rstd::move(input));
        }));
    if (committed.is_none()) {
        SettingsError error;
        error.message = "Settings changed; reload before saving"_Str;
        co_return Ok(ProviderMutationOutcome::SettingsRejected(rstd::move(error)));
    }
    SettingsSnapshot snapshot { u64(committed->to_primitive()), rstd::move(settings_values) };
    if (replace_fields) {
        auto replacement = temporary ? temporary->instance.clone() : fresh.clone();
        co_return Ok(ProviderMutationOutcome::Committed(
            ProviderCommittedMutation { ProviderMutationKind::Updated,
                                        provider_id,
                                        ProviderLiveMutation::Replace(rstd::move(old),
                                                                      rstd::move(replacement),
                                                                      rstd::move(temporary),
                                                                      rstd::move(name),
                                                                      rstd::move(new_key),
                                                                      rstd::move(snapshot)) }));
    }
    co_return Ok(ProviderMutationOutcome::Committed(ProviderCommittedMutation {
        ProviderMutationKind::Updated,
        provider_id,
        ProviderLiveMutation::Update(rstd::move(old), rstd::move(name), rstd::move(snapshot)) }));
}

auto provider_update_request(const p_qcm::p_msg::M_UpdateProviderReq& entry)
    -> ProcessResult<ProviderUpdateRequest> {
    auto id             = rstd_try(required_id(entry.f_provider_id, "provider_id"_str));
    auto authentication = ProviderAuthenticationUpdate::Unchanged();
    if (entry.f_auth_info.is_some()) {
        if (entry.f_tmp_provider.is_empty()) {
            authentication = ProviderAuthenticationUpdate::Authenticate(
                rstd::into<AuthInput>((*entry.f_auth_info).as_ref()));
        } else {
            authentication = ProviderAuthenticationUpdate::Conflict();
        }
    }
    return Ok(ProviderUpdateRequest { id,
                                      entry.f_name.clone(),
                                      entry.f_tmp_provider.clone(),
                                      entry.f_settings_json.clone(),
                                      entry.f_settings_revision,
                                      rstd::move(authentication) });
}

auto replace_provider_async(TmpProviderStore& store, ProviderRuntime& runtime,
                            SyncCoordinator& coordinator, const ProviderRegistry& registry,
                            sqlito::ExecutorHandle                    db,
                            const p_qcm::p_msg::M_ReplaceProviderReq& entry,
                            ProviderMutationWrite&                    write)
    -> rstd::async::coro<ProcessResult<ProviderMutationOutcome>> {
    auto provider_id = rstd_co_try(required_id(entry.f_provider_id, "provider_id"_str));
    if (entry.f_tmp_provider.is_empty())
        co_return Err(ProcessError::MissingFields(String::make("tmp_provider"_str)));
    auto stored = rstd_co_try(
        co_await database_run(db.clone(), [provider_id](sqlito::Connection& connection) {
            return read_provider_settings(connection, provider_id);
        }));
    ProviderUpdateRequest update { provider_id,
                                   rstd::move(stored.name),
                                   entry.f_tmp_provider.clone(),
                                   rstd::move(stored.config),
                                   u64(static_cast<unsigned long long>(
                                       stored.revision.to_primitive())),
                                   ProviderAuthenticationUpdate::Unchanged() };
    auto                  result = rstd_co_try(co_await update_provider_async(
        store, runtime, coordinator, registry, rstd::move(db), rstd::move(update), write));
    RSTD_MATCH(result) {
        RSTD_CASE(Committed, mutation) {
            mutation.kind = ProviderMutationKind::Replaced;
            co_return Ok(rstd::move(result));
        }
        RSTD_CASE(SettingsRejected, error) {
            co_return Err(ProcessError::internal(error.message.as_str()));
        }
        RSTD_CASE(AuthenticationRejected, outcome) {
            if (outcome.is_Failed())
                co_return Err(ProcessError::internal(outcome.as_Failed().message.as_str()));
            co_return Err(ProcessError::internal(""_str));
        }
    }
}

auto delete_provider_async(sqlito::ExecutorHandle db, SyncCoordinator& coordinator,
                           const p_qcm::p_msg::M_DeleteProviderReq& entry,
                           ProviderMutationWrite&                   write)
    -> rstd::async::coro<ProcessResult<ProviderMutationOutcome>> {
    auto id = rstd_co_try(required_id(entry.f_provider_id, "provider_id"_str));
    rstd_co_try(
        co_await write.run(rstd::move(db),
                           [id, coordinator = coordinator.clone()](
                               sqlito::Connection& connection) mutable -> ProcessResult<empty> {
                               return delete_provider_record(connection, coordinator, id);
                           }));
    co_return Ok(ProviderMutationOutcome::Committed(ProviderCommittedMutation {
        ProviderMutationKind::Deleted, id, ProviderLiveMutation::Delete() }));
}

auto provider_mutation_response(i32 request_id, ProviderMutationOutcome outcome)
    -> p_qcm::p_msg::M_QcmMessage {
    RSTD_MATCH(outcome) {
        RSTD_CASE(Committed, mutation) {
            return mutation.kind == ProviderMutationKind::Updated
                       ? update_auth_response(request_id, AuthOutcome::Ok())
                       : provider_ok(request_id);
        }
        RSTD_CASE(SettingsRejected, error) {
            return update_settings_response(request_id, rstd::move(error));
        }
        RSTD_CASE(AuthenticationRejected, auth) {
            return update_auth_response(request_id, rstd::move(auth));
        }
    }
}
auto load_radio_queues_async(ProviderRuntime& runtime, sqlito::ExecutorHandle db,
                             SyncCoordinator& coordinator, Vec<i64> providers,
                             ProviderRequestOwnerHandle owner = {})
    -> rstd::async::coro<Result<Vec<Box<p_qcm::p_msg::p_model::M_RadioQueue>>, ProcessError>> {
    Vec<Box<p_qcm::p_msg::p_model::M_RadioQueue>> queues;
    auto                                          rows = rstd_co_try(co_await database_run(
        db.clone(), [providers = rstd::move(providers)](sqlito::Connection& connection) {
            return radio_queue_rows(connection, providers.as_slice());
        }));
    Vec<ProviderSnapshot>                         snapshots;
    for (const auto& row : rows.as_slice()) {
        auto provider_id = row.provider_id;
        auto instance    = runtime.acquire(provider_id);
        if (! instance)
            co_return Err(ProcessError::NoSuchProvider(rstd::format("{}", provider_id)));
        auto epoch = instance->session_epoch();
        if (epoch.is_none()) co_return Err(ProcessError::NotAuth());
        auto call = runtime.call_snapshot(provider_id, instance, epoch);
        if (call.is_none()) co_return Err(ProcessError::NotAuth());
        snapshots.push(call->clone());
        auto context = call->context(db.clone(), coordinator.clone());
        if (owner) {
            owner.get()->provider_id = provider_id;
            owner.get()->instance    = instance;
        }
        auto definition = co_await instance.get()->queue_definition_async(rstd::move(context),
                                                                          row.native_id.clone());
        if (owner) owner.get()->instance.reset();
        if (! call->is_current()) co_return Err(ProcessError::NotAuth());
        auto radio_definition = rstd_co_try(rstd::move(definition));
        auto queue            = Box<p_qcm::p_msg::p_model::M_RadioQueue>::make();
        auto queue_id         = Box<p_qcm::p_msg::p_id::M_RadioQueueId>::make();
        queue_id->f_value     = row.id;
        queue->f_id           = Some(rstd::move(queue_id));
        queue->f_name         = row.name.clone();
        queue->f_description  = row.description.clone();
        for (auto& item : radio_definition.configuration)
            queue->f_configuration.push(
                rstd::into<Box<p_qcm::p_msg::p_model::M_QueueConfigField>>(rstd::move(item)));
        queue->f_can_restart   = radio_definition.can_restart;
        queue->f_restart_label = rstd::move(radio_definition.restart_label);
        queues.push(rstd::move(queue));
    }
    for (const auto& snapshot : snapshots.as_slice()) {
        if (! snapshot.is_current()) co_return Err(ProcessError::NotAuth());
    }
    co_return Ok(rstd::move(queues));
}
auto get_radio_queues_async(ProviderRuntime& runtime, sqlito::ExecutorHandle db,
                            SyncCoordinator& coordinator, i32 request_id,
                            const p_qcm::p_msg::M_GetRadioQueuesReq& entry,
                            ProviderRequestOwnerHandle               owner = {})
    -> rstd::async::coro<Result<p_qcm::p_msg::M_QcmMessage, ProcessError>> {
    auto providers = rstd_co_try(checked_ids(entry.f_provider_ids, "provider_ids"_str));
    auto queues    = rstd_co_try(co_await load_radio_queues_async(
        runtime, rstd::move(db), coordinator, rstd::move(providers), owner));
    auto rsp       = Box<p_qcm::p_msg::M_GetRadioQueuesRsp>::make();
    rsp->f_queues  = rstd::move(queues);
    co_return Ok(qcm_message(request_id, rstd::move(rsp)));
}
auto qr_response(i32 request_id, QrInfo info) -> Result<p_qcm::p_msg::M_QcmMessage, ProcessError> {
    auto rsp   = Box<p_qcm::p_msg::M_QrAuthUrlRsp>::make();
    rsp->f_key = rstd::move(info.key);
    rsp->f_url = rstd::move(info.url);
    return Ok(qcm_message(request_id, rstd::move(rsp)));
}
auto qr_auth_url_async(TmpProviderStore& store, i32 request_id,
                       const p_qcm::p_msg::M_QrAuthUrlReq& entry)
    -> rstd::async::coro<Result<p_qcm::p_msg::M_QcmMessage, ProcessError>> {
    auto tmp = store.acquire(entry.f_tmp_provider.as_str());
    if (! tmp) co_return Err(ProcessError::NoSuchProvider(entry.f_tmp_provider.clone()));
    auto info = rstd_co_try(co_await tmp.get()->instance.get()->qr_async());
    co_return qr_response(request_id, rstd::move(info));
}
auto valid_share_url(const lihttpto::Url& url) -> bool {
    return url.host().is_some() && url.userinfo().is_none();
}
auto get_share_link_async(ProviderRuntime& runtime, sqlito::ExecutorHandle db, i32 request_id,
                          const p_qcm::p_msg::M_GetShareLinkReq& entry)
    -> rstd::async::coro<Result<p_qcm::p_msg::M_QcmMessage, ProcessError>> {
    auto rsp      = Box<p_qcm::p_msg::M_GetShareLinkRsp>::make();
    auto resolved = rstd_co_try(co_await resolve_provider_item(
        rstd::move(db), runtime, entry.f_id, i64(entry.f_item_type.to_primitive())));
    if (resolved.is_some()) {
        auto& target = *resolved;
        auto  url    = rstd_co_try(
            runtime.share_url(target.provider_id, target.native_id.as_str(), target.type));
        if (url.is_some()) {
            if (! valid_share_url(*url))
                co_return Err(ProcessError::internal("invalid share url"_str));
            rsp->f_url = Some(String::make(url->as_ref()));
        }
    }
    co_return Ok(qcm_message(request_id, rstd::move(rsp)));
}
} // namespace qcm::backend
