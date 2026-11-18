export module qcm.backend:plugin.manifest;
export import :plugin.config;
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
} // namespace qcm::backend
