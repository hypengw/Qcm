use once_cell::sync::Lazy;
use reqwest::{ClientBuilder, Proxy, Url};
use serde::{Deserialize, Serialize};
use std::{
    collections::BTreeMap,
    sync::{Arc, RwLock},
};

#[derive(Clone, Default, Serialize, Deserialize)]
pub struct ProxyEntry {
    #[serde(default)]
    pub id: String,
    pub name: String,
    pub url: String,
    #[serde(default)]
    pub provider_ids: Vec<i64>,
    #[serde(default, skip_deserializing)]
    pub display_url: String,
}
#[derive(Clone, Default, Serialize, Deserialize)]
pub struct NetworkSettings {
    pub revision: u64,
    pub proxies: Vec<ProxyEntry>,
}
#[derive(Clone, Default)]
pub struct ClientPolicy {
    key: Option<String>,
    proxy: Option<Proxy>,
}
impl PartialEq for ClientPolicy {
    fn eq(&self, other: &Self) -> bool {
        self.key == other.key
    }
}
impl ClientPolicy {
    pub fn builder(&self) -> ClientBuilder {
        let builder = ClientBuilder::new().no_proxy();
        match &self.proxy {
            Some(proxy) => builder.proxy(proxy.clone()),
            None => builder,
        }
    }
}
pub struct CompiledSettings {
    pub settings: NetworkSettings,
    routes: BTreeMap<i64, ClientPolicy>,
}
static ROUTES: Lazy<RwLock<Arc<BTreeMap<i64, ClientPolicy>>>> =
    Lazy::new(|| RwLock::new(Arc::new(BTreeMap::new())));

pub fn policy(provider_id: Option<i64>) -> ClientPolicy {
    provider_id
        .and_then(|id| ROUTES.read().unwrap().get(&id).cloned())
        .unwrap_or_default()
}
pub fn publish(compiled: CompiledSettings) {
    *ROUTES.write().unwrap() = Arc::new(compiled.routes);
}
pub fn compile(mut settings: NetworkSettings) -> Result<CompiledSettings, String> {
    if settings.proxies.len() > 64 {
        return Err("Too many proxies".into());
    }
    let mut ids = std::collections::BTreeSet::new();
    let mut routes = BTreeMap::new();
    for entry in &mut settings.proxies {
        entry.id = if entry.id.is_empty() {
            uuid::Uuid::new_v4()
        } else {
            uuid::Uuid::parse_str(&entry.id).map_err(|_| "Invalid proxy ID")?
        }
        .to_string();
        if !ids.insert(entry.id.clone()) {
            return Err("Duplicate proxy ID".into());
        }
        entry.name = entry.name.trim().to_owned();
        if entry.name.is_empty() || entry.name.len() > 128 {
            return Err("Proxy name must contain 1–128 bytes".into());
        }
        if entry.url.len() > 4096 {
            return Err("Proxy address is too long".into());
        }
        let mut url = Url::parse(entry.url.trim()).map_err(|_| "Invalid proxy address")?;
        if !matches!(url.scheme(), "http" | "https" | "socks5" | "socks5h")
            || url.host_str().is_none()
            || url.port_or_known_default().is_none()
            || url.port() == Some(0)
            || !matches!(url.path(), "" | "/")
            || url.query().is_some()
            || url.fragment().is_some()
        {
            return Err(
                "Use an http, https, socks5 or socks5h proxy address with a valid host and port"
                    .into(),
            );
        }
        entry.url = url.to_string();
        let proxy = Proxy::all(&entry.url).map_err(|_| "Invalid proxy address")?;
        url.set_username("").map_err(|_| "Invalid proxy address")?;
        url.set_password(None)
            .map_err(|_| "Invalid proxy address")?;
        entry.display_url = url.to_string();
        if entry.provider_ids.len() > 256 {
            return Err("Too many providers for a proxy".into());
        }
        let route = ClientPolicy {
            key: Some(entry.url.clone()),
            proxy: Some(proxy),
        };
        for &id in &entry.provider_ids {
            if id <= 0 || routes.insert(id, route.clone()).is_some() {
                return Err("A provider may use only one proxy".into());
            }
        }
    }
    Ok(CompiledSettings { settings, routes })
}

#[cfg(test)]
mod tests {
    use super::*;
    fn settings(url: &str) -> NetworkSettings {
        NetworkSettings {
            revision: 0,
            proxies: vec![ProxyEntry {
                name: " Test ".into(),
                url: url.into(),
                provider_ids: vec![91],
                ..Default::default()
            }],
        }
    }
    #[test]
    fn proxy_validation_and_redaction() {
        for url in [
            "http://user:secret@localhost:7890",
            "https://localhost",
            "socks5://localhost:1080",
            "socks5h://[::1]:1080",
        ] {
            let compiled = compile(settings(url)).unwrap();
            assert_eq!(compiled.settings.proxies[0].name, "Test");
            assert!(!compiled.settings.proxies[0].display_url.contains("secret"));
            assert!(compiled.routes[&91] != ClientPolicy::default());
            compiled.routes[&91].builder().build().unwrap();
        }
        for url in [
            "not a URL",
            "file:///tmp/proxy",
            "http://localhost:0",
            "http://localhost/path",
            "http://localhost?query=1",
            "socks5://localhost",
        ] {
            assert!(compile(settings(url)).is_err());
        }
        let mut duplicate = settings("http://localhost:7890");
        duplicate.proxies.push(duplicate.proxies[0].clone());
        assert!(compile(duplicate).is_err());
        let mut invalid = settings("http://localhost:7890");
        invalid.proxies[0].provider_ids = vec![0];
        assert!(compile(invalid).is_err());
        let mut invalid = settings("http://localhost:7890");
        invalid.proxies[0].name.clear();
        assert!(compile(invalid).is_err());
    }
}
