use futures_util::{SinkExt, StreamExt};
use prost::Message;
use std::io::Write;

#[allow(dead_code)]
#[path = "../src/msg.rs"]
mod msg;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let listener = tokio::net::TcpListener::bind("127.0.0.1:0").await?;
    println!("{{\"port\":{}}}", listener.local_addr()?.port());
    std::io::stdout().flush()?;
    let (socket, _) = listener.accept().await?;
    let mut socket = tokio_tungstenite::accept_async(socket).await?;
    let Some(request) = socket.next().await else {
        return Ok(());
    };
    let request = msg::QcmMessage::decode(request?.into_data().as_ref())?;
    let mode = std::env::var("QCM_PEER_MODE").unwrap_or_else(|_| "old".into());
    if mode == "timeout" {
        while socket.next().await.is_some() {}
        return Ok(());
    }
    let early = mode == "early";
    let response = msg::QcmMessage {
        protocol_version: if mode == "old" { 0 } else { 2 },
        id: if early { 0 } else { request.id },
        r#type: if early {
            msg::MessageType::ProviderStatusMsg
        } else {
            msg::MessageType::TestRsp
        } as i32,
        payload: Some(if early {
            msg::qcm_message::Payload::ProviderStatusMsg(Default::default())
        } else {
            msg::qcm_message::Payload::TestRsp(Default::default())
        }),
        entities: Some(msg::entity::EntityBatch {
            protocol: if mode == "entity" { 1 } else { 2 },
            instance: "protocol-peer".into(),
            ..Default::default()
        }),
    };
    socket
        .send(tokio_tungstenite::tungstenite::Message::Binary(
            response.encode_to_vec().into(),
        ))
        .await?;
    while socket.next().await.is_some() {}
    Ok(())
}
