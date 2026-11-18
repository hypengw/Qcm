module;
#include <rstd/macro.hpp>
module qcm.backend;
import :provider.lua_plugin;
import :provider.registry;
import :provider.lua;
import :plugin.manifest;
import rstd;
import rstd.log;
using rstd::path::Path;
using rstd::path::PathBuf;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend
{
namespace
{
auto load_manifest(ref<Path> plugin_dir) -> ProviderResult<ProviderMetaInfo> {
    auto             manifest_path = PathBuf::from(plugin_dir).join("plugin.json"_str);
    auto             manifest      = rstd_try(PluginManifest::read(manifest_path.as_path()));
    auto             config        = manifest.config();
    auto             svg_path      = PathBuf::from(plugin_dir).join(manifest.svg_path.as_str());
    auto             script_path   = PathBuf::from(plugin_dir).join(manifest.script_path.as_str());
    ProviderMetaInfo meta;
    meta.type_name      = rstd::move(manifest.name);
    meta.is_script      = true;
    meta.has_server_url = manifest.has_server_url.unwrap_or(true);
    meta.auth_types     = rstd::move(manifest.auth_types);
    meta.sync           = config.sync();
    meta.svg =
        rstd_try(rstd::fs::read_to_string(svg_path.as_path()), ProviderError::io(rstd::move(_e)));
    meta.creator =
        ProviderFactory::make([script = rstd::move(script_path), config = rstd::move(config)](
                                  ProviderOptions options) -> ProviderResult<ProviderHandle> {
            options.config = config.clone();
            return make_lua_provider(script.as_path(), rstd::move(options));
        });
    return Ok(rstd::move(meta));
}

} // namespace

auto LuaProviderPlugin::load_dir(ref<Path> dir) -> ProviderResult<ProviderRegistry> {
    ProviderRegistry registry;
    auto iterator = rstd_try(rstd::fs::read_dir(dir), ProviderError::io(rstd::move(_e)));
    for (;;) {
        auto next = iterator.next();
        if (next.is_none()) break;
        auto entry = rstd::move(*next);
        if (entry.is_err()) {
            rstd::log::warn("cannot read provider entry: {}", rstd::move(entry).unwrap_err());
            continue;
        }
        auto path   = entry->path();
        auto loaded = load_manifest(path.as_path());
        if (loaded.is_err()) {
            rstd::log::warn("cannot load provider manifest: {}", loaded.as_ref().unwrap_err());
            continue;
        }
        auto registered = registry.register_provider(rstd::move(*loaded));
        if (registered.is_err())
            rstd::log::warn("cannot register provider: {}", registered.as_ref().unwrap_err());
    }
    return Ok(rstd::move(registry));
}
auto LuaProviderPlugin::load_default() -> ProviderResult<ProviderRegistry> {
    ProviderRegistry registry;
    auto collect = [&registry](ProviderResult<ProviderRegistry> result) -> ProviderResult<empty> {
        if (result.is_err()) {
            auto error = rstd::move(result).unwrap_err();
            if (error.is_Io() && error.as_Io().cause.kind().code == rstd::io::ErrorKind::NotFound)
                return Ok(empty {});
            return Err(rstd::move(error));
        }
        for (const auto& meta : result->metas()) {
            if (registry.find(meta.type_name.as_str()) != nullptr) continue;
            rstd_try(registry.register_provider(meta.clone()));
        }
        return Ok(empty {});
    };
    auto overridden = rstd::env::var("QCM_PLUGIN_DIR"_str);
    if (overridden.is_ok()) {
        auto path = PathBuf::from(overridden->as_str());
        rstd_try(collect(load_dir(path.as_path())));
    }
    auto executable = rstd::fs::read_link(PathBuf::from("/proc/self/exe"_str).as_path());
    if (executable.is_ok()) {
        auto parent = executable->as_path().parent();
        if (parent.is_some()) {
            auto plugins = PathBuf::from(*parent);
            auto child   = PathBuf::from("plugins"_str);
            plugins.push(child.as_path());
            rstd_try(collect(load_dir(plugins.as_path())));
        }
    }
    auto system = PathBuf::from("/usr/share/QcmPlugin"_str);
    rstd_try(collect(load_dir(system.as_path())));
    return Ok(rstd::move(registry));
}
} // namespace qcm::backend
