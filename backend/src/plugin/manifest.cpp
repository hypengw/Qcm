module;
#include <rstd/macro.hpp>
module qcm.backend;
import :plugin.manifest;
import rstd.json;
using rstd::path::Path;
using namespace rstd::prelude;

namespace qcm::backend
{
auto PluginManifest::config() const -> PluginConfig {
    return { api_version, sync_mode.clone(), sync_types.clone() };
}
auto PluginManifest::parse(ref<str> text) -> ProviderResult<PluginManifest> {
    auto value =
        rstd_try(rstd::json::decode<PluginManifest>(text), ProviderError::serde(rstd::move(_e)));
    rstd_try(value.config().validate());
    value.name.as_mut_str().make_ascii_lowercase();
    return Ok(rstd::move(value));
}
auto PluginManifest::read(ref<Path> path) -> ProviderResult<PluginManifest> {
    auto text = rstd_try(rstd::fs::read_to_string(path), ProviderError::io(rstd::move(_e)));
    return parse(text.as_str());
}
} // namespace qcm::backend
