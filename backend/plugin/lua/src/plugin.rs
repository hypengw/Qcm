use super::provider::LuaProvider;
use qcm_core::provider::{Creator, Provider, ProviderMeta};
use qcm_core::{plugin::Plugin, Result};
use serde::{Deserialize, Serialize};
use std::env;
use std::fs;
use std::path::{Path, PathBuf};
use std::sync::Arc;

#[derive(Deserialize, Serialize)]
struct PluginJson {
    name: String,
    script_path: PathBuf,
    svg_path: PathBuf,
    has_server_url: Option<bool>,
    auth_types: Vec<i32>,
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
        if let Ok(content) = fs::read_to_string(path.join("plugin.json")) {
            if let Ok(mut plugin_json) = serde_json::from_str::<PluginJson>(&content) {
                plugin_json.name = plugin_json.name.to_ascii_lowercase();
                if let Ok(svg_content) = fs::read_to_string(path.join(&plugin_json.svg_path)) {
                    return Some((plugin_json, svg_content));
                }
            }
        }
        None
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
                LuaProvider::new(id, name, device_id, &type_name, &script_path).map(|p| {
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
