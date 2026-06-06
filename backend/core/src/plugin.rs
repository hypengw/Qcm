use crate::provider::ProviderMeta;
pub struct PluginContext {}

pub trait Plugin: Send {
    fn id(&self) -> &str;
    fn name(&self) -> &str;

    fn provider_metas(&self) -> Vec<ProviderMeta>;
}
