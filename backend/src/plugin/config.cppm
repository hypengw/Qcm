module;
#include <rstd/macro.hpp>
export module qcm.backend:plugin.config;
export import :provider.dto;
export import :provider.error;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
inline constexpr u32 plugin_api_version = u32(1);

struct [[pmacro::derive("serde::derive")]] PluginConfig {
    u32 api_version { plugin_api_version };
    [[pmacro::helper("serde::derive", default)]]
    Option<String> sync_mode;
    [[pmacro::helper("serde::derive", default)]]
    Vec<ItemType> sync_types;

    auto clone() const -> PluginConfig;
    auto explicit_sync() const -> bool;
    auto validate() const -> ProviderResult<empty>;
    auto sync() const -> ProviderSyncConfig;
};
inline auto PluginConfig::clone() const -> PluginConfig {
    return { api_version, sync_mode.clone(), sync_types.clone() };
}
inline auto PluginConfig::explicit_sync() const -> bool {
    return sync_mode.is_some() && sync_mode->as_str() == "explicit"_str;
}
inline auto PluginConfig::validate() const -> ProviderResult<empty> {
    if (api_version != plugin_api_version) {
        auto message = rstd::format(
            "unsupported Lua plugin api_version: {}; expected {}", api_version, plugin_api_version);
        return Err(ProviderError::internal(message.as_str()));
    }
    if (sync_mode.is_some() && sync_mode->as_str() != "explicit"_str &&
        sync_mode->as_str() != "legacy"_str)
        return Err(ProviderError::internal("unknown Lua sync_mode"_str));
    if (explicit_sync() && sync_types.is_empty())
        return Err(ProviderError::internal("explicit sync requires sync_types"_str));
    return Ok(empty {});
}
inline auto PluginConfig::sync() const -> ProviderSyncConfig {
    return { explicit_sync() ? ProviderSyncMode::Explicit : ProviderSyncMode::Legacy,
             sync_types.clone() };
}
} // namespace qcm::backend
