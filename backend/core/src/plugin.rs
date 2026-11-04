use crate::provider::ProviderMeta;
use sea_orm::DatabaseConnection;
use serde::Serialize;
use std::sync::{Arc, LazyLock};

static RUNTIME_ID: LazyLock<String> = LazyLock::new(|| uuid::Uuid::new_v4().to_string());

mod state;
pub use state::{InstallationState, StateUpdate, MAX_STATE_BYTES};

#[derive(Clone, Debug, Serialize)]
pub struct PlatformInfo {
    pub os: &'static str,
    pub arch: &'static str,
    pub family: &'static str,
    pub source: &'static str,
}

impl PlatformInfo {
    pub fn current() -> Self {
        Self {
            os: std::env::consts::OS,
            arch: std::env::consts::ARCH,
            family: std::env::consts::FAMILY,
            source: "rust_target",
        }
    }
}

#[derive(Clone)]
pub struct PluginContext {
    namespace: Arc<str>,
    installation_id: Arc<str>,
    platform: PlatformInfo,
    db: DatabaseConnection,
}

impl PluginContext {
    pub fn new(
        namespace: &str,
        installation_id: &str,
        db: DatabaseConnection,
    ) -> crate::Result<Self> {
        if namespace.is_empty()
            || namespace.len() > 128
            || !namespace
                .bytes()
                .all(|c| c.is_ascii_alphanumeric() || b"._-".contains(&c))
        {
            return Err(crate::anyhow!("invalid plugin state namespace"));
        }
        if installation_id.is_empty() {
            return Err(crate::anyhow!("installation identity is unavailable"));
        }
        Ok(Self {
            namespace: namespace.into(),
            installation_id: installation_id.into(),
            platform: PlatformInfo::current(),
            db,
        })
    }

    pub fn namespace(&self) -> &str {
        &self.namespace
    }

    pub fn installation_id(&self) -> &str {
        &self.installation_id
    }

    pub fn runtime_id(&self) -> &str {
        &RUNTIME_ID
    }

    pub fn platform(&self) -> &PlatformInfo {
        &self.platform
    }

    pub async fn load_state(&self) -> crate::Result<InstallationState> {
        state::load(&self.db, &self.namespace).await
    }

    pub async fn compare_exchange_state(
        &self,
        revision: u64,
        data: String,
    ) -> crate::Result<StateUpdate> {
        state::compare_exchange(&self.db, &self.namespace, revision, data).await
    }
}

pub trait Plugin: Send {
    fn id(&self) -> &str;
    fn name(&self) -> &str;

    fn provider_metas(&self) -> Vec<ProviderMeta>;
}
