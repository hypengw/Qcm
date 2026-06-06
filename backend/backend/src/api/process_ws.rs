use super::process_qcm::process_qcm;
pub use hyper_tungstenite::tungstenite::Message as WsMessage;
use qcm_core::Result;
use std::sync::Arc;
use tokio::sync::mpsc::Sender;

use crate::error::ProcessError;
use crate::event::ServiceContext;
use crate::msg::QcmMessage;

type TX = Sender<WsMessage>;

pub async fn process_ws(
    ctx: &Arc<ServiceContext>,
    tx: &TX,
    msg: &WsMessage,
    in_id: &mut Option<i32>,
) -> Result<QcmMessage, ProcessError> {
    match msg {
        WsMessage::Binary(data) => {
            let mut qcm_msg = process_qcm(ctx, data, in_id).await;
            if let (Some(id), Ok(qcm_msg)) = (in_id, &mut qcm_msg) {
                qcm_msg.id = *id;
            };
            return qcm_msg;
        }
        WsMessage::Text(text) => {
            log::info!("{}", text);
            tx.send(WsMessage::Text(format!("Echo: {}", text).into()))
                .await
                .unwrap();
            return Err(ProcessError::None);
        }
        WsMessage::Ping(a) => {
            tx.send(WsMessage::Pong(a.clone())).await.unwrap();
            return Err(ProcessError::None);
        }
        _ => {
            return Err(ProcessError::None);
        }
    }
}
