use once_cell::sync::Lazy;
use sea_orm::EntityTrait;
use std::collections::BTreeMap;
use std::sync::{Arc, Mutex};

use crate::event::ServiceContext;

struct Global {
    contexts: BTreeMap<i64, Arc<ServiceContext>>,
    shutdown_tx: Option<tokio::sync::watch::Sender<bool>>,
}

impl Global {
    fn new() -> Self {
        Self {
            contexts: BTreeMap::new(),
            shutdown_tx: None,
        }
    }
}

static GLOBAL: Lazy<Arc<Mutex<Global>>> = Lazy::new(|| Arc::new(Mutex::new(Global::new())));

pub fn context(port: i64) -> Option<Arc<ServiceContext>> {
    let g = GLOBAL.lock().unwrap();
    g.contexts.get(&port).cloned()
}

pub fn reg_context(port: i64, c: Arc<ServiceContext>) {
    let mut g = GLOBAL.lock().unwrap();
    g.contexts.insert(port, c);
}

pub fn unreg_context(port: i64) {
    let mut g = GLOBAL.lock().unwrap();
    g.contexts.remove(&port);
}

pub fn set_shutdown_tx(tx: tokio::sync::watch::Sender<bool>) {
    let mut g = GLOBAL.lock().unwrap();
    g.shutdown_tx = Some(tx);
}

pub fn shutdown() {
    let g = GLOBAL.lock().unwrap();
    if let Some(tx) = &g.shutdown_tx {
        let _ = tx.send(true);
    }
}
