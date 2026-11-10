module;
#include <rstd/macro.hpp>
export module qcm.backend:provider.lua_impl;
import luato;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend
{
auto optional_lua_function(luato::TableHandle& table, ref<str> name)
    -> luato::Result<Option<luato::FunctionHandle>> {
    auto value = rstd_try(table.get_value(name));
    if (value.is_Nil()) return Ok(None());
    if (! value.is_Function())
        return Err(luato::Error::make(luato::ErrorKind::Type,
                                      "provider"_Str,
                                      rstd::format("{} must be a function or nil", name)));
    return Ok(Some(rstd::move(value).as_Function().value));
}
auto required_lua_function(luato::TableHandle& table, ref<str> name)
    -> luato::Result<luato::FunctionHandle> {
    auto function = rstd_try(optional_lua_function(table, name));
    if (function.is_none())
        return Err(luato::Error::make(
            luato::ErrorKind::Type, "provider"_Str, rstd::format("{} func not found", name)));
    return Ok(rstd::move(*function));
}

struct LuaImpl {
    Option<luato::FunctionHandle> comments;
    Option<luato::FunctionHandle> comment_replies;
    Option<luato::FunctionHandle> set_comment_like;
    Option<luato::FunctionHandle> validate_settings;
    Option<luato::FunctionHandle> share_url;
    luato::FunctionHandle         load;
    luato::FunctionHandle         save;
    luato::FunctionHandle         check;
    luato::FunctionHandle         login;
    luato::FunctionHandle         sync;
    luato::FunctionHandle         sync_item;
    Option<luato::FunctionHandle> get_queue_next;
    Option<luato::FunctionHandle> queue_configuration;
    Option<luato::FunctionHandle> queue_definition;
    Option<luato::FunctionHandle> open_queue;
    Option<luato::FunctionHandle> next_queue;
    Option<luato::FunctionHandle> close_queue;
    luato::FunctionHandle         favorite;
    Option<luato::FunctionHandle> qr;
    Option<luato::FunctionHandle> prepare_image;
    Option<luato::FunctionHandle> prepare_audio;
    Option<luato::FunctionHandle> open_playback;
    Option<luato::FunctionHandle> prepare_playback_resource;
    Option<luato::FunctionHandle> keep_playback;
    Option<luato::FunctionHandle> close_playback;
    luato::FunctionHandle         subtitle;
    Option<luato::FunctionHandle> for_you_items;
    Option<luato::FunctionHandle> home_blocks;
    Option<luato::FunctionHandle> home_block_items;

    static auto read(luato::TableHandle& table) -> luato::Result<LuaImpl> {
        auto open_playback = rstd_try(optional_lua_function(table, "open_playback"_str));
        auto prepare_playback_resource =
            rstd_try(optional_lua_function(table, "prepare_playback_resource"_str));
        auto keep_playback  = rstd_try(optional_lua_function(table, "keep_playback"_str));
        auto close_playback = rstd_try(optional_lua_function(table, "close_playback"_str));
        auto playback_any   = open_playback.is_some() || prepare_playback_resource.is_some() ||
                              keep_playback.is_some() || close_playback.is_some();
        auto playback_all   = open_playback.is_some() && prepare_playback_resource.is_some() &&
                              keep_playback.is_some() && close_playback.is_some();
        if (playback_any && ! playback_all)
            return Err(luato::Error::binding(
                "playback requires open_playback, prepare_playback_resource, keep_playback and close_playback together"_Str));
        auto queue_definition = rstd_try(optional_lua_function(table, "queue_definition"_str));
        auto open_queue       = rstd_try(optional_lua_function(table, "open_queue"_str));
        auto next_queue       = rstd_try(optional_lua_function(table, "next_queue"_str));
        auto close_queue      = rstd_try(optional_lua_function(table, "close_queue"_str));
        auto queue_any = queue_definition.is_some() || open_queue.is_some() || next_queue.is_some();
        auto queue_all = queue_definition.is_some() && open_queue.is_some() && next_queue.is_some();
        if (queue_any && ! queue_all)
            return Err(luato::Error::binding(
                "radio requires queue_definition, open_queue and next_queue together"_Str));
        if (close_queue.is_some() && ! queue_all)
            return Err(luato::Error::binding(
                "close_queue requires queue_definition, open_queue and next_queue"_Str));

        return Ok(LuaImpl {
            .comments          = rstd_try(optional_lua_function(table, "comments"_str)),
            .comment_replies   = rstd_try(optional_lua_function(table, "comment_replies"_str)),
            .set_comment_like  = rstd_try(optional_lua_function(table, "set_comment_like"_str)),
            .validate_settings = rstd_try(optional_lua_function(table, "validate_settings"_str)),
            .share_url         = rstd_try(optional_lua_function(table, "share_url"_str)),
            .load              = rstd_try(required_lua_function(table, "load"_str)),
            .save              = rstd_try(required_lua_function(table, "save"_str)),
            .check             = rstd_try(required_lua_function(table, "check"_str)),
            .login             = rstd_try(required_lua_function(table, "login"_str)),
            .sync              = rstd_try(required_lua_function(table, "sync"_str)),
            .sync_item         = rstd_try(required_lua_function(table, "sync_item"_str)),
            .get_queue_next    = rstd_try(optional_lua_function(table, "get_queue_next"_str)),
            .queue_configuration =
                rstd_try(optional_lua_function(table, "queue_configuration"_str)),
            .queue_definition = rstd::move(queue_definition),
            .open_queue       = rstd::move(open_queue),
            .next_queue       = rstd::move(next_queue),
            .close_queue      = rstd::move(close_queue),
            .favorite         = rstd_try(required_lua_function(table, "favorite"_str)),
            .qr               = table.get_function("qr"_str).ok(),
            .prepare_image    = rstd_try(optional_lua_function(table, "prepare_image"_str)),
            .prepare_audio    = rstd_try(optional_lua_function(table, "prepare_audio"_str)),
            .open_playback    = rstd::move(open_playback),
            .prepare_playback_resource = rstd::move(prepare_playback_resource),
            .keep_playback             = rstd::move(keep_playback),
            .close_playback            = rstd::move(close_playback),
            .subtitle                  = rstd_try(required_lua_function(table, "subtitle"_str)),
            .for_you_items    = rstd_try(optional_lua_function(table, "for_you_items"_str)),
            .home_blocks      = table.get_function("home_blocks"_str).ok(),
            .home_block_items = table.get_function("home_block_items"_str).ok(),
        });
    }
};
} // namespace qcm::backend
