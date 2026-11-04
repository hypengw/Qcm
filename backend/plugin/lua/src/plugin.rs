use super::provider::LuaProvider;
use qcm_core::provider::{Creator, Provider, ProviderMeta};
use qcm_core::{plugin::Plugin, Result};
use serde::{Deserialize, Serialize};
use std::env;
use std::fs;
use std::path::{Path, PathBuf};
use std::sync::Arc;

pub const API_VERSION: u32 = 1;

#[derive(Clone, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct PluginConfig {
    pub api_version: u32,
    #[serde(default)]
    pub sync_mode: Option<String>,
    #[serde(default)]
    pub sync_types: Vec<qcm_core::model::type_enum::ItemType>,
}
impl Default for PluginConfig {
    fn default() -> Self {
        Self {
            api_version: API_VERSION,
            sync_mode: None,
            sync_types: Vec::new(),
        }
    }
}

impl PluginConfig {
    pub fn explicit(&self) -> bool {
        self.sync_mode.as_deref() == Some("explicit")
    }
    pub fn validate(&self) -> Result<()> {
        if self.api_version != API_VERSION {
            return Err(qcm_core::anyhow!(
                "unsupported Lua plugin api_version: {}; expected {}",
                self.api_version,
                API_VERSION
            ));
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
#[serde(deny_unknown_fields)]
pub struct PluginManifest {
    name: String,
    script_path: PathBuf,
    svg_path: PathBuf,
    has_server_url: Option<bool>,
    auth_types: Vec<i32>,
    api_version: u32,
    #[serde(default)]
    sync_mode: Option<String>,
    #[serde(default)]
    sync_types: Vec<qcm_core::model::type_enum::ItemType>,
}

impl PluginManifest {
    pub fn read(path: &Path) -> Result<Self> {
        let mut manifest: Self = serde_json::from_str(&fs::read_to_string(path)?)?;
        manifest.config().validate()?;
        manifest.name.make_ascii_lowercase();
        Ok(manifest)
    }

    pub fn config(&self) -> PluginConfig {
        PluginConfig {
            api_version: self.api_version,
            sync_mode: self.sync_mode.clone(),
            sync_types: self.sync_types.clone(),
        }
    }
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

    fn load_plugin_json(&self, path: &Path) -> Option<(PluginManifest, String)> {
        let manifest = path.join("plugin.json");
        if !manifest.is_file() {
            return None;
        }
        let load = || -> Result<(PluginManifest, String)> {
            let json = PluginManifest::read(&manifest)?;
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
        plugin_json: PluginManifest,
        svg_content: String,
    ) -> ProviderMeta {
        let config = plugin_json.config();
        let script_path = path.join(&plugin_json.script_path);
        let type_name = plugin_json.name.clone();
        let creator: Arc<Creator> =
            Arc::new(move |id, name, device_id| -> Result<Arc<dyn Provider>> {
                LuaProvider::with_context(
                    id,
                    name,
                    device_id,
                    &type_name,
                    &script_path,
                    config.clone(),
                    qcm_core::global::plugin_context(&type_name)?,
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
