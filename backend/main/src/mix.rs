use crate::error::ProcessError;
use qcm_core::{global, provider::Context};
use std::{
    collections::HashMap,
    sync::{Arc, Mutex, Weak},
};
use tokio::sync::Mutex as AsyncMutex;

#[derive(Default)]
pub struct MixRefresh {
    locks: Mutex<HashMap<i64, Weak<AsyncMutex<()>>>>,
}

impl MixRefresh {
    pub async fn refresh(&self, ctx: &Context, id: i64, force: bool) -> Result<(), ProcessError> {
        let lock = {
            let mut locks = self.locks.lock().unwrap();
            locks.retain(|_, lock| lock.strong_count() > 0);
            match locks.get(&id).and_then(Weak::upgrade) {
                Some(lock) => lock,
                None => {
                    let lock = Arc::new(AsyncMutex::new(()));
                    locks.insert(id, Arc::downgrade(&lock));
                    lock
                }
            }
        };
        let _guard = lock.lock().await;
        if let Some((_, item)) =
            qcm_core::db::sync::remote_mix_for_refresh(&ctx.db, id, force).await?
        {
            let provider = global::provider(item.provider_id)
                .ok_or_else(|| ProcessError::NoSuchProvider(item.provider_id.to_string()))?;
            provider.sync_item(ctx, item).await?;
        }
        Ok(())
    }
}

#[cfg(test)]
mod tests;
