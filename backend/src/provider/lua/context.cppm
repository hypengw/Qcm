module;
#include <rstd/enum.hpp>
#include <rstd/macro.hpp>
export module qcm.backend:provider.lua.context;
export import :provider.lua.support;
using alloc::rc::Rc;
using rstd::tuple;
using rstd::async::coro;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
namespace bindings
{
struct LuaContext {
    rstd::sync::Weak<BindingsContext> owner;
    Rc<LuaCallState>                  call;
    auto clone() const -> LuaContext { return { owner.clone(), call.clone() }; }
};
struct LuaRun {
    LuaContext context;
};
inline auto ctx_call(const BindingsContextArc& context, const luato::UserDataHandle& self)
    -> Result<Rc<LuaCallState>, luato::Error> {
    auto read = [&](const LuaContext& object) -> Result<Rc<LuaCallState>, luato::Error> {
        auto owner = object.owner.upgrade();
        if (! owner || rstd::addressof(*owner) != rstd::addressof(*context))
            return Err(fail("context belongs to another provider"_str));
        rstd_try(object.call->check());
        return Ok(object.call.clone());
    };
    if (rstd_try(self.is<LuaContext>())) return self.with<LuaContext>(read);
    return self.with<LuaRun>([&](const LuaRun& run) {
        return read(run.context);
    });
}
inline auto deserialize_options(const BindingsContext& context) -> luato::DeserializeOptions {
    return context.json.is_some() ? (*context.json)->deserialize_options()
                                  : luato::DeserializeOptions {};
}
template<class T>
inline auto ctx_decode(const BindingsContext& context, const luato::LuaValue& value)
    -> Result<T, luato::Error> {
    return luato::from_value<T>(value, deserialize_options(context));
}
inline auto legacy_sync_options(const BindingsContext&         context,
                                const Option<luato::LuaValue>& value)
    -> Result<LegacySyncOptions, luato::Error> {
    if (value.is_none()) return Ok(LegacySyncOptions {});
    return ctx_decode<LegacySyncOptions>(context, *value);
}
struct LuaSyncScope {
    Option<i64> library_id;
    i64         item_type {};
};
constexpr auto rstd_serde_definition(LuaSyncScope*) {
    using namespace rstd::serde;
    return RecordDefinition<LuaSyncScope,
                            "SyncScope",
                            Direction::Deserialize,
                            UnknownFieldPolicy::Reject,
                            Member<&LuaSyncScope::library_id, "library_id", FieldPolicy::Defaulted>,
                            Member<&LuaSyncScope::item_type, "item_type">> {};
}
struct LuaBeginSync {
    Vec<i64>          libraries;
    Vec<LuaSyncScope> scopes;
};
constexpr auto rstd_serde_definition(LuaBeginSync*) {
    using namespace rstd::serde;
    return RecordDefinition<LuaBeginSync,
                            "BeginSync",
                            Direction::Deserialize,
                            UnknownFieldPolicy::Reject,
                            Member<&LuaBeginSync::libraries, "libraries">,
                            Member<&LuaBeginSync::scopes, "scopes">> {};
}
inline auto sync_scope(const BindingsContext& context, const luato::LuaValue& value)
    -> Result<ObserveScope, luato::Error> {
    auto decoded = rstd_try(ctx_decode<LuaSyncScope>(context, value));
    return Ok(ObserveScope { decoded.library_id, decoded.item_type, false });
}
inline auto ctx_inject_provider(luato::TableHandle& items, i64 provider)
    -> Result<empty, luato::Error> {
    auto length = rstd_try(items.length());
    auto keys   = rstd_try(items.keys());
    if (keys.len() != length) return Err(fail("items must be a dense array"_str));
    for (const auto& key : keys)
        if (! key.is_Integer() || key.as_Integer().value < i64(1) ||
            key.as_Integer().value > i64(length.to_primitive()))
            return Err(fail("items must be a dense array"_str));
    for (usize index {}; index < length; ++index) {
        auto item =
            rstd_try(items.get_in_call(luato::LuaValue::Integer(i64(index.to_primitive() + 1))));
        if (! item.is_Table()) return Err(fail("item must be a table"_str));
        auto& table = item.as_Table().value;
        auto  owner = rstd_try(table.get_in_call(str_key("provider_id"_str)));
        if (owner.is_Nil())
            rstd_try(
                table.set_in_call(str_key("provider_id"_str), luato::LuaValue::Integer(provider)));
        else if (! owner.is_Integer() || owner.as_Integer().value != provider)
            return Err(fail("item belongs to another provider"_str));
    }
    return Ok(empty {});
}
inline auto table_input(const luato::LuaValue& value) -> Result<luato::TableHandle, luato::Error> {
    return luato::FromLua<luato::TableHandle>::from_lua(rstd_try(value.clone()));
}
inline auto ctx_get_playing(BindingsContextArc context, luato::CallFrame& frame)
    -> luato::BindingResult {
    auto self = rstd_try(luato::argument<luato::UserDataHandle>(frame, usize()));
    auto call = rstd_try(ctx_call(context, self));
    (void)rstd_try(call->provider_id());
    auto provider = context->common->id();
    if (provider.is_none()) {
        frame.push_nil();
        return Ok(usize(1));
    }
    auto info = context->playing->get(*provider);
    if (info.is_none()) {
        frame.push_nil();
        return Ok(usize(1));
    }
    auto value = rstd_try(new_table(frame));
    rstd_try(value.set_in_call(str_key("song_id"_str), luato::LuaValue::Integer(info->song_id)));
    rstd_try(value.set_in_call(str_key("native_id"_str), str_value(info->native_id.as_str())));
    return_table(frame, rstd::move(value));
    return Ok(usize());
}
template<class Build>
inline auto ctx_execute(BindingsContextArc context, luato::UserDataHandle self, Build build,
                        bool returns_run = false)
    -> coro<Result<luato::AsyncReturn, luato::Error>> {
    auto call    = rstd_co_try(ctx_call(context, self));
    auto command = build(*context, call);
    if (command.is_err()) {
        call.get()->fail_sync();
        co_return Err(rstd::move(command).unwrap_err());
    }
    auto output   = rstd_co_try(co_await LuaCallState::execute(call.clone(), rstd::move(*command)));
    auto failed   = call.clone();
    auto returned = luato::AsyncReturn::make(
        [context = context.clone(),
         call    = rstd::move(call),
         output  = rstd::move(output),
         returns_run](luato::CallFrame& frame) mutable -> luato::BindingResult {
            rstd_try(call->check());
            if (returns_run) {
                auto run = rstd_try(frame.create_userdata(
                    LuaRun { LuaContext { context.downgrade(), call.clone() } }));
                return luato::return_value(frame, rstd::move(run));
            }
            if (output.is_Unit()) {
                frame.push_nil();
                return Ok(usize(1));
            }
            if (context->json.is_none())
                return Err(fail("run result serialization context is unavailable"_str));
            auto options = luato::SerializeOptions::from_deserialize_options(
                (*context->json)->deserialize_options());
            // DB results mark sequences, but leave records and maps unmarked.
            options.object_metatable       = nullptr;
            options.serialize_unit_to_null = false;
            return luato::return_value(frame, luato::to_value(frame, output, options));
        });
    co_return Ok(
        rstd::move(returned).on_error([call = rstd::move(failed)](const luato::Error&) mutable {
            call.get()->fail_sync();
        }));
}
inline auto ctx_argument_failure(BindingsContextArc context) {
    return [context = rstd::move(context)](const luato::UserDataHandle& self,
                                           const luato::Error&,
                                           luato::AsyncMethodErrorKind kind) {
        if (kind != luato::AsyncMethodErrorKind::Arguments) return;
        auto call = ctx_call(context, self);
        if (call.is_ok()) call->get()->fail_sync();
    };
}
template<class... Args, class T, class Build>
inline auto ctx_method(luato::UserDataSpec<T>& object, BindingsContextArc context, String name,
                       Build build, bool returns_run = false) -> void {
    auto failure = ctx_argument_failure(context.clone());
    object.async_method(
        rstd::move(name),
        // Non-legacy context methods ignore surplus Lua arguments.
        [context = rstd::move(context), build, returns_run](
            luato::UserDataHandle self,
            Args... inputs,
            luato::Variadic<luato::LuaValue>) -> coro<Result<luato::AsyncReturn, luato::Error>> {
            return ctx_execute(
                context.clone(),
                rstd::move(self),
                [build, ... inputs = rstd::move(inputs)](
                    const BindingsContext&  context,
                    const Rc<LuaCallState>& call) mutable -> Result<LuaRunCommand, luato::Error> {
                    return build(context, call, rstd::move(inputs)...);
                },
                returns_run);
        },
        rstd::move(failure));
}
template<class T>
inline auto register_context(BindingsContextArc context, luato::State& state)
    -> Result<empty, luato::Error> {
    constexpr bool is_run = rstd::mtp::same_as<T, LuaRun>;
    auto           object = luato::UserDataSpec<T>(is_run ? "qcm.run"_Str : "qcm.context"_Str);
    ctx_method<luato::LuaValue>(
        object,
        context.clone(),
        "sync_batch"_Str,
        [](const BindingsContext& context,
           const Rc<LuaCallState>&,
           luato::LuaValue batch) -> Result<LuaRunCommand, luato::Error> {
            return Ok(LuaRunCommand::Batch(rstd_try(ctx_decode<SyncBatch>(context, batch))));
        });
    ctx_method<luato::LuaValue, luato::LuaValue>(
        object,
        context.clone(),
        "observe_items"_Str,
        [](const BindingsContext& context,
           const Rc<LuaCallState>&,
           luato::LuaValue scope_value,
           luato::LuaValue items_value) -> Result<LuaRunCommand, luato::Error> {
            auto scope = rstd_try(sync_scope(context, scope_value));
            auto items = rstd_try(table_input(items_value));
            if (rstd_try(items.length()) > usize(200000))
                return Err(fail("sync input exceeds entry limit"_str));
            return lua_run::observe_items(
                rstd::move(scope), items_value, deserialize_options(context));
        });
    ctx_method<luato::LuaValue>(object,
                                context.clone(),
                                "observe_libraries"_Str,
                                [](const BindingsContext& context,
                                   const Rc<LuaCallState>&,
                                   luato::LuaValue ids) -> Result<LuaRunCommand, luato::Error> {
                                    return Ok(LuaRunCommand::ObserveLibraries(
                                        rstd_try(ctx_decode<Vec<i64>>(context, ids))));
                                });
    ctx_method<luato::LuaValue>(
        object,
        context.clone(),
        "complete"_Str,
        [](const BindingsContext& context,
           const Rc<LuaCallState>&,
           luato::LuaValue scope) -> Result<LuaRunCommand, luato::Error> {
            RSTD_MATCH(scope) {
                RSTD_CASE(Bytes) {
                    auto text = rstd_try(scope.as_utf8());
                    if (text != "libraries"_str) return Err(fail("invalid completion scope"_str));
                    return Ok(LuaRunCommand::CompleteLibraries());
                }
                RSTD_CASE(Table) {
                    return Ok(LuaRunCommand::Complete(rstd_try(sync_scope(context, scope))));
                }
                RSTD_CASE_DEFAULT { return Err(fail("invalid completion scope"_str)); }
            }
        });
    ctx_method(
        object,
        context.clone(),
        "seal"_Str,
        [](const BindingsContext&, const Rc<LuaCallState>&) -> Result<LuaRunCommand, luato::Error> {
            return Ok(LuaRunCommand::Seal());
        });
    if constexpr (! is_run) {
        object.raw_method("get_playing"_Str, [context = context.clone()](luato::CallFrame& frame) {
            return ctx_get_playing(context.clone(), frame);
        });
        ctx_method<luato::LuaValue>(
            object,
            context.clone(),
            "begin_sync"_Str,
            [](const BindingsContext& context,
               const Rc<LuaCallState>&,
               luato::LuaValue value) -> Result<LuaRunCommand, luato::Error> {
                auto options = rstd_try(ctx_decode<LuaBeginSync>(context, value));
                if (options.libraries.len() > usize(200000) || options.scopes.len() > usize(200000))
                    return Err(fail("sync declaration exceeds entry limit"_str));
                Vec<ObserveScope> scopes;
                for (const auto& scope : options.scopes)
                    scopes.push({ scope.library_id, scope.item_type, false });
                return Ok(LuaRunCommand::Begin(rstd::move(options.libraries), rstd::move(scopes)));
            },
            true);
        ctx_method(object,
                   context.clone(),
                   "libraries"_Str,
                   [](const BindingsContext&,
                      const Rc<LuaCallState>&) -> Result<LuaRunCommand, luato::Error> {
                       return Ok(LuaRunCommand::Libraries());
                   });
        ctx_method<String, luato::LuaValue>(
            object,
            context.clone(),
            "resolve_items"_Str,
            [](const BindingsContext& context,
               const Rc<LuaCallState>&,
               String          kind,
               luato::LuaValue values) -> Result<LuaRunCommand, luato::Error> {
                auto items = rstd_try(table_input(values));
                if (rstd_try(items.length()) > usize(500))
                    return Err(fail("sync input exceeds entry limit"_str));
                return lua_run::resolve_items(kind.as_str(), values, deserialize_options(context));
            });
        ctx_method<luato::LuaValue>(
            object,
            context.clone(),
            "allocate_items"_Str,
            [](const BindingsContext&  context,
               const Rc<LuaCallState>& call,
               luato::LuaValue         values) -> Result<LuaRunCommand, luato::Error> {
                auto items = rstd_try(table_input(values));
                rstd_try(ctx_inject_provider(items, rstd_try(call->provider_id())));
                return lua_run::allocate_items(
                    values, rstd_try(call->provider_id()), deserialize_options(context));
            });
        ctx_method<i64, luato::LuaValue>(
            object,
            context.clone(),
            "sync_remote_mix_song_ids"_Str,
            [](const BindingsContext& context,
               const Rc<LuaCallState>&,
               i64             mix,
               luato::LuaValue values) -> Result<LuaRunCommand, luato::Error> {
                auto items = rstd_try(table_input(values));
                return lua_run::sync_remote_mix_song_ids(mix, values, deserialize_options(context));
            });
        ctx_method<i64, luato::LuaValue>(
            object,
            context.clone(),
            "sync_song_album_ids"_Str,
            [](const BindingsContext& context,
               const Rc<LuaCallState>&,
               i64             library,
               luato::LuaValue values) -> Result<LuaRunCommand, luato::Error> {
                auto pairs = rstd_try((ctx_decode<Vec<tuple<String, String>>>(context, values)));
                Vec<LegacySongAlbumInput> inputs;
                for (auto& pair : pairs)
                    inputs.push({ rstd::move(rstd::get<0>(pair)), rstd::move(rstd::get<1>(pair)) });
                return Ok(LuaRunCommand::SyncSongAlbumIds(library, rstd::move(inputs)));
            });
        auto progress = [&](String name, SyncProgressKind kind) {
            ctx_method<i64>(object,
                            context.clone(),
                            rstd::move(name),
                            [kind](const BindingsContext&,
                                   const Rc<LuaCallState>&,
                                   i64 count) -> Result<LuaRunCommand, luato::Error> {
                                auto number = count.to_primitive();
                                if (number < -2147483648LL || number > 2147483647LL)
                                    return Err(fail("commit count is outside i32 range"_str));
                                return Ok(
                                    LuaRunCommand::Progress(kind, i32(static_cast<int>(number))));
                            });
        };
        progress("commit_album"_Str, SyncProgressKind::Album);
        progress("commit_artist"_Str, SyncProgressKind::Artist);
        progress("commit_song"_Str, SyncProgressKind::Song);
        auto array = [&](String name, auto decode) {
            ctx_method<luato::LuaValue>(
                object,
                context.clone(),
                rstd::move(name),
                [decode](const BindingsContext& context,
                         const Rc<LuaCallState>&,
                         luato::LuaValue values) -> Result<LuaRunCommand, luato::Error> {
                    auto items = rstd_try(table_input(values));
                    return decode(values, deserialize_options(context));
                });
        };
        array("sync_radio_queues"_Str, lua_run::sync_radio_queues);
        array("sync_album_artist_ids"_Str, lua_run::sync_album_artist_ids);
        array("sync_song_artist_ids"_Str, lua_run::sync_song_artist_ids);
        array("sync_artists"_Str, lua_run::sync_artists);
        array("sync_images"_Str, lua_run::sync_images);
        array("sync_songs"_Str, lua_run::sync_songs);
        array("sync_libraries"_Str, lua_run::sync_libraries);
        auto legacy = [&](String name, auto decode) {
            object.async_method(
                rstd::move(name),
                [context = context.clone(),
                 decode](luato::UserDataHandle self,
                         luato::LuaValue       values,
                         Option<luato::LuaValue>
                             options) -> coro<Result<luato::AsyncReturn, luato::Error>> {
                    return ctx_execute(
                        context.clone(),
                        rstd::move(self),
                        [decode, values = rstd::move(values), options = rstd::move(options)](
                            const BindingsContext& context,
                            const Rc<LuaCallState>&) -> Result<LuaRunCommand, luato::Error> {
                            auto items        = rstd_try(table_input(values));
                            auto sync_options = rstd_try(legacy_sync_options(context, options));
                            return decode(
                                values, rstd::move(sync_options), deserialize_options(context));
                        });
                },
                ctx_argument_failure(context.clone()));
        };
        legacy("sync_remote_mixes"_Str, lua_run::sync_remote_mixes);
        legacy("sync_dynamics"_Str, lua_run::sync_dynamics);
        legacy("sync_albums"_Str, lua_run::sync_albums);
    }
    return state.register_userdata(rstd::move(object));
}
inline auto install_context(BindingsContextArc context, luato::State& state)
    -> Result<empty, luato::Error> {
    rstd_try(register_context<LuaContext>(context.clone(), state));
    return register_context<LuaRun>(context.clone(), state);
}
} // namespace bindings
} // namespace qcm::backend
