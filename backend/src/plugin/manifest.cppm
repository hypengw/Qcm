module;
#include <rstd/macro.hpp>
export module qcm.backend:plugin.manifest;
export import :plugin.config;
import rstd.json;
using rstd::path::Path;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
struct [[pmacro::derive("serde::derive")]] PluginManifest {
    String name;
    String script_path;
    String svg_path;
    [[pmacro::helper("serde::derive", default)]]
    Option<bool> has_server_url;
    Vec<i32>     auth_types;
    u32          api_version {};
    [[pmacro::helper("serde::derive", default)]]
    Option<String> sync_mode;
    [[pmacro::helper("serde::derive", default)]]
    Vec<ItemType> sync_types;

    auto        config() const -> PluginConfig;
    static auto parse(ref<str> text) -> ProviderResult<PluginManifest>;
    static auto read(ref<Path> path) -> ProviderResult<PluginManifest>;
};

inline auto PluginManifest::config() const -> PluginConfig {
    return { api_version, sync_mode.clone(), sync_types.clone() };
}
inline auto PluginManifest::parse(ref<str> text) -> ProviderResult<PluginManifest> {
    auto value =
        rstd_try(rstd::json::decode<PluginManifest>(text), ProviderError::serde(rstd::move(_e)));
    rstd_try(value.config().validate());
    value.name.as_mut_str().make_ascii_lowercase();
    return Ok(rstd::move(value));
}
inline auto PluginManifest::read(ref<Path> path) -> ProviderResult<PluginManifest> {
    auto text = rstd_try(rstd::fs::read_to_string(path), ProviderError::io(rstd::move(_e)));
    return parse(text.as_str());
}
} // namespace qcm::backend
