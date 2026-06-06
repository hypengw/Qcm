use super::process_http::process_http_get;
use super::process_ws::process_ws;
pub use super::process_ws::WsMessage;
use crate::api::process_http::process_http_post;
use crate::convert::*;
use crate::global as bglobal;
use crate::http::body_type::ResponseBody;
use crate::msg::{self, QcmMessage, Rsp};
use crate::reverse::ReverseEvent;
use crate::task::TaskManagerOper;
use crate::{
    error::ProcessError,
    event::{self, EventSink, ServiceContext, BackendEvent},
};
use futures_util::{SinkExt, StreamExt};
use http_body_util::BodyExt;
use hyper::body::Incoming;
use hyper::{Request, Response, StatusCode};
use hyper_tungstenite::HyperWebsocket;
use prost::{self, Message};
use qcm_core::provider::Context;
use qcm_core::Result;
use scopeguard::guard;
use sea_orm::DatabaseConnection;
use std::sync::Arc;
use tokio::sync::mpsc as async_mpsc;

use super::process_event::{process_backend_event, process_event, ProcessContext};

/// WebSocket 传输层的 EventSink 实现
struct WsEventSink {
    tx: async_mpsc::Sender<WsMessage>,
}

impl EventSink for WsEventSink {
    fn send_message(
        &self,
        msg: QcmMessage,
    ) -> std::pin::Pin<Box<dyn std::future::Future<Output = qcm_core::Result<()>> + Send + '_>>
    {
        Box::pin(async move {
            let mut buf = Vec::new();
            msg.encode(&mut buf)?;
            self.tx.send(WsMessage::Binary(buf.into())).await?;
            Ok(())
        })
    }
}

pub async fn handle_request(
    mut request: Request<Incoming>,
    db: DatabaseConnection,
    cache_db: DatabaseConnection,
    oper: TaskManagerOper,
    reverse_ev: tokio::sync::mpsc::Sender<ReverseEvent>,
) -> Result<Response<ResponseBody>> {
    // if ws
    if hyper_tungstenite::is_upgrade_request(&request) {
        let (response, websocket) = hyper_tungstenite::upgrade(&mut request, None)?;

        // spawn to handle ws connect
        tokio::spawn(async move {
            if let Err(e) = handle_ws(websocket, db, cache_db, oper, reverse_ev).await {
                log::error!("Error in websocket connection: {e}");
            }
        });

        // return upgrade rsp
        Ok(response.map(|b| ResponseBody::Boxed(b.map_err(|e| e.into()).boxed())))
    } else {
        // TODO: support multiple backend
        let ctx = bglobal::context(1).unwrap();
        log::debug!(target: "http", "{}", request.uri());

        use hyper::Method;
        let res = match *request.method() {
            Method::GET => process_http_get(&ctx, request).await,
            Method::POST => process_http_post(&ctx, request).await,
            _ => {
                let rsp = Response::builder()
                    .status(StatusCode::METHOD_NOT_ALLOWED)
                    .body(ResponseBody::Empty)
                    .unwrap();
                Ok(rsp)
            }
        };

        match res {
            Ok(rsp) => Ok(rsp),
            Err(
                ProcessError::NotFound
                | ProcessError::NoSuchAlbum(_)
                | ProcessError::NoSuchArtist(_)
                | ProcessError::NoSuchSong(_)
                | ProcessError::NoSuchMix(_)
                | ProcessError::NoSuchItemType(_),
            ) => {
                let rsp = Response::builder()
                    .status(StatusCode::NOT_FOUND)
                    .body(ResponseBody::Empty)
                    .unwrap();
                Ok(rsp)
            }
            Err(err) => Err(err.into()),
        }
    }
}

async fn handle_ws(
    ws: HyperWebsocket,
    db: DatabaseConnection,
    cache_db: DatabaseConnection,
    oper: TaskManagerOper,
    reverse_ev: tokio::sync::mpsc::Sender<ReverseEvent>,
) -> Result<()> {
    let (mut ws_writer, ws_reader) = ws.await?.split();

    let (ws_sender, mut ws_receiver) = async_mpsc::channel::<WsMessage>(256);
    let (ev_sender, mut ev_receiver) = async_mpsc::channel::<event::Event>(1024);
    let (bk_ev_sender, mut bk_ev_receiver) = async_mpsc::channel::<event::BackendEvent>(1024);

    let sink: Arc<dyn EventSink> = Arc::new(WsEventSink { tx: ws_sender.clone() });

    let ctx = Arc::new(ServiceContext {
        provider_context: Arc::new(Context {
            db,
            cache_db,
            ev_sender: ev_sender,
        }),
        backend_ev: bk_ev_sender,
        oper,
        reverse_ev,
    });

    // TODO: reg context by real port
    bglobal::reg_context(1, ctx.clone());
    let _guard = guard(1, |p| {
        bglobal::unreg_context(p);
    });

    // ws sender queue
    tokio::spawn({
        async move {
            while let Some(msg) = ws_receiver.recv().await {
                if ws_writer.send(msg.into()).await.is_err() {
                    break;
                }
            }
            log::info!("Channel recv end");
        }
    });

    // event queue
    tokio::spawn({
        let ctx = ctx.clone();
        async move {
            while let Some(ev) = ev_receiver.recv().await {
                match process_event(ev, ctx.clone()).await {
                    Ok(true) => break,
                    Err(err) => log::error!("{}", err),
                    _ => (),
                }
            }
            log::info!("Event channel recv end");
        }
    });

    // backend event queue
    tokio::spawn({
        let ctx = ctx.clone();
        let sink = sink.clone();
        async move {
            let mut process_ctx = ProcessContext::new();
            while let Some(ev) = bk_ev_receiver.recv().await {
                match process_backend_event(ev, ctx.clone(), sink.clone(), &mut process_ctx).await {
                    Ok(true) => break,
                    Err(err) => log::error!("{}", err),
                    _ => (),
                }
            }
            log::info!("Backend event channel recv end");
        }
    });

    let _ = ctx.backend_ev.try_send(BackendEvent::Frist);

    // receive from ws
    let mut reader = ws_reader;
    while let Ok(Some(message)) = reader.next().await.transpose() {
        tokio::spawn({
            let ctx = ctx.clone();
            let ws_sender = ws_sender.clone();
            async move {
                if let Err(e) = handle_ws_message(message, ctx, ws_sender).await {
                    log::warn!("Error processing message: {}", e);
                }
            }
        });
    }

    log::info!("WebSocket connection closed");

    // end event process
    ctx.provider_context
        .ev_sender
        .send(event::Event::End)
        .await
        .unwrap();
    ctx.backend_ev.send(event::BackendEvent::End).await.unwrap();

    // Only support one client, close self
    bglobal::shutdown();
    return Ok(());
}

pub async fn handle_ws_message(
    msg: WsMessage,
    ctx: Arc<ServiceContext>,
    ws_sender: async_mpsc::Sender<WsMessage>,
) -> Result<()> {
    use msg::qcm_message::Payload;
    let mut id: Option<i32> = None;

    match process_ws(&ctx, &ws_sender, &msg, &mut id).await {
        Ok(msg_rsp) => {
            log::warn!("send {}", msg_rsp.r#type);
            let mut buf = Vec::new();
            msg_rsp.encode(&mut buf)?;
            ws_sender.send(WsMessage::Binary(buf.into())).await?;
        }
        Err(ProcessError::None) => {}
        Err(err) => {
            if let Some(id) = id {
                let rsp: Rsp = err.qcm_into();
                let msg = QcmMessage {
                    id,
                    r#type: msg::MessageType::Rsp.into(),
                    payload: Some(Payload::Rsp(rsp)),
                };
                log::warn!("send {}", msg.r#type);
                let mut buf = Vec::new();
                msg.encode(&mut buf)?;
                ws_sender.send(WsMessage::Binary(buf.into())).await?;
            } else {
                log::error!("{}", err);
            }
        }
    }
    Ok(())
}
