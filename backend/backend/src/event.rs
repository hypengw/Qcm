use super::reverse::ReverseEvent;
use super::task::TaskManagerOper;
pub use qcm_core::event::Event;
pub use qcm_core::event::SyncCommit;
use qcm_core::{self, provider};
use std::sync::Arc;
use tokio::sync::mpsc::Sender;

use crate::msg::QcmMessage;

/// 传输层推送通道抽象
/// 不同 IPC（WebSocket、Unix socket 等）各自实现
pub trait EventSink: Send + Sync {
    fn send_message(
        &self,
        msg: QcmMessage,
    ) -> std::pin::Pin<Box<dyn std::future::Future<Output = qcm_core::Result<()>> + Send + '_>>;
}

pub enum BackendEvent {
    Frist,
    NewProvider { id: i64 },
    UpdateProvider { id: i64 },
    DeleteProvider { id: i64 },
    ReplaceProvider { id: i64 },
    SyncCommit { id: i64, commit: SyncCommit },
    End,
}

/// 传输无关的服务上下文，可被任意 IPC 层复用
pub struct ServiceContext {
    pub provider_context: Arc<provider::Context>,
    pub backend_ev: Sender<BackendEvent>,
    pub oper: TaskManagerOper,
    pub reverse_ev: Sender<ReverseEvent>,
}
