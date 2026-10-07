use super::provider::LuaProvider;
use qcm_core::provider::{Creator, Provider, ProviderMeta};
use qcm_core::{plugin::Plugin, Result};
use serde::{Deserialize, Serialize};
use std::env;
use std::fs;
use std::path::{Path, PathBuf};
use std::sync::Arc;

pub const CAPABILITIES: &[&str] = &[
    "http.request.v1",
    "http.response.v1",
    "http.batch.response.v1",
    "json.strict.v1",
    "db.sync_batch.v1",
    "db.explicit_sync.v1",
    "subtitle.structured.v1",
    "provider.metadata.v1",
];
#[derive(Clone, Default, Deserialize, Serialize)]
pub struct Capabilities {
    #[serde(default)]
    pub requires: Vec<String>,
    #[serde(default)]
    pub sync_mode: Option<String>,
    #[serde(default)]
    pub sync_types: Vec<qcm_core::model::type_enum::ItemType>,
}
impl Capabilities {
    pub fn explicit(&self) -> bool {
        self.sync_mode.as_deref() == Some("explicit")
    }
    pub fn validate(&self) -> Result<()> {
        for required in &self.requires {
            if !CAPABILITIES.contains(&required.as_str()) {
                return Err(qcm_core::anyhow!(
                    "unsupported Lua capability: {}",
                    required
                ));
            }
        }
        if self
            .sync_mode
            .as_deref()
            .is_some_and(|mode| mode != "explicit" && mode != "legacy")
        {
            return Err(qcm_core::anyhow!("unknown Lua sync_mode"));
        }
        if self.explicit() && self.sync_types.is_empty() {
            return Err(qcm_core::anyhow!("explicit sync requires sync_types"));
        }
        Ok(())
    }
}

#[derive(Deserialize, Serialize)]
struct PluginJson {
    name: String,
    script_path: PathBuf,
    svg_path: PathBuf,
    has_server_url: Option<bool>,
    auth_types: Vec<i32>,
    #[serde(flatten)]
    capabilities: Capabilities,
}

pub struct LuaPlugin {
    plugins_dir: Vec<String>,
}

impl LuaPlugin {
    pub fn new() -> Self {
        let mut plugin_dirs = Vec::new();

        if let Ok(exec_path) = env::current_exe() {
            if let Some(exec_dir) = exec_path.parent() {
                plugin_dirs.push(exec_dir.join("plugins").to_string_lossy().to_string());
            }
        }

        #[cfg(target_os = "linux")]
        plugin_dirs.push("/usr/share/QcmPlugin".to_string());

        Self {
            plugins_dir: plugin_dirs,
        }
    }

    fn load_plugin_json(&self, path: &Path) -> Option<(PluginJson, String)> {
        let manifest = path.join("plugin.json");
        if !manifest.is_file() {
            return None;
        }
        let load = || -> Result<(PluginJson, String)> {
            let mut json: PluginJson = serde_json::from_str(&fs::read_to_string(&manifest)?)?;
            json.name = json.name.to_ascii_lowercase();
            json.capabilities.validate()?;
            let svg = fs::read_to_string(path.join(&json.svg_path))?;
            Ok((json, svg))
        };
        match load() {
            Ok(plugin) => Some(plugin),
            Err(error) => {
                log::error!("{}: {error}", manifest.display());
                None
            }
        }
    }

    fn provider_creator(
        &self,
        path: &Path,
        plugin_json: PluginJson,
        svg_content: String,
    ) -> ProviderMeta {
        let script_path = path.join(plugin_json.script_path);
        let type_name = plugin_json.name.clone();
        let creator: Arc<Creator> =
            Arc::new(move |id, name, device_id| -> Result<Arc<dyn Provider>> {
                LuaProvider::with_capabilities(
                    id,
                    name,
                    device_id,
                    &type_name,
                    &script_path,
                    plugin_json.capabilities.clone(),
                )
                .map(|p| {
                    let p: Arc<dyn Provider> = Arc::new(p);
                    p
                })
            });

        let mut meta = ProviderMeta::new(
            &plugin_json.name,
            &plugin_json.auth_types,
            Arc::new(svg_content),
            creator,
        );
        meta.is_script = true;
        meta.has_server_url = plugin_json.has_server_url.unwrap_or(true);
        return meta;
    }
}

impl Plugin for LuaPlugin {
    fn id(&self) -> &str {
        return "qcm.plugin.lua";
    }
    fn name(&self) -> &str {
        return "lua";
    }
    fn provider_metas(&self) -> Vec<ProviderMeta> {
        let mut metas = Vec::new();

        for plugins_dir in &self.plugins_dir {
            if let Ok(entries) = fs::read_dir(plugins_dir) {
                for entry in entries.flatten() {
                    let path = entry.path();
                    if let Some((plugin_json, svg_content)) = self.load_plugin_json(&path) {
                        metas.push(self.provider_creator(&path, plugin_json, svg_content));
                    }
                }
            }
        }

        metas
    }
}
