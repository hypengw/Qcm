use crate::event::{EventSink, ServiceContext, BackendEvent, Event};
use crate::msg::model::AuthInfo;
use qcm_core::event::SyncCommit;
use qcm_core::event::{Event as CoreEvent, SyncState};
use qcm_core::model as sqlm;
use qcm_core::provider::Provider;
use qcm_core::{self};
use qcm_core::{global, Result};
use sea_orm::EntityTrait;
use std::collections::BTreeMap;
use std::sync::Arc;
use tokio::sync::oneshot;

use crate::convert::*;
use crate::msg::{
    self, model::ProviderStatus, ProviderMetaStatusMsg, ProviderStatusMsg,
    QcmMessage,
};

pub struct ProcessContext {
    sync_status: BTreeMap<i64, msg::model::ProviderSyncStatus>,
}

impl ProcessContext {
    pub fn new() -> Self {
        Self {
            sync_status: BTreeMap::new(),
        }
    }
}

pub async fn process_event(ev: Event, ctx: Arc<ServiceContext>) -> Result<bool> {
    match ev {
        Event::ProviderSync { id, oneshot } => {
            if let Some(p) = global::provider(id) {
                let name = p.name();
                ctx.oper.spawn({
                    let ctx = ctx.provider_context.clone();
                    log::info!("spawn sync: {}", name);
                    |tid| async move {
                        log::info!("start sync: {}", name);
                        if let Some(tx) = oneshot {
                            let _ = tx.send(tid);
                        }
                        let id = p.id().unwrap();
                        let _ = ctx
                            .ev_sender
                            .send(CoreEvent::SyncCommit {
                                id,
                                commit: SyncCommit::SetState(SyncState::Syncing),
                            })
                            .await;

                        match p.sync(&ctx).await {
                            Err(err) => {
                                log::error!("{:?}", err);
                                let _ = ctx
                                    .ev_sender
                                    .send(CoreEvent::SyncCommit {
                                        id,
                                        commit: SyncCommit::SetState(err.into()),
                                    })
                                    .await;
                            }
                            Ok(_) => {
                                let _ = ctx
                                    .ev_sender
                                    .send(CoreEvent::SyncCommit {
                                        id,
                                        commit: SyncCommit::SetState(SyncState::Finished),
                                    })
                                    .await;
                            }
                        }
                        log::info!("sync end: {}", name);
                    }
                });
            }
        }
        Event::SyncCommit { id, commit } => {
            let _ = ctx
                .backend_ev
                .send(BackendEvent::SyncCommit { id, commit })
                .await;
        }
        Event::End => return Ok(true),
    }
    return Ok(false);
}

pub async fn process_backend_event(
    ev: BackendEvent,
    ctx: Arc<ServiceContext>,
    sink: Arc<dyn EventSink>,
    pctx: &mut ProcessContext,
) -> Result<bool> {
    let ev_sender = &ctx.provider_context.ev_sender;
    match ev {
        BackendEvent::Frist => {
            send_provider_meta_status(ctx.as_ref(), sink.as_ref()).await?;
            let _ = send_provider_status(ctx.as_ref(), sink.as_ref(), &pctx.sync_status).await?;
            // for p in providers {
            //     if let Some(id) = p.id() {
            //         ev_sender.send(Event::ProviderSync { id: id }).await?;
            //     }
            // }
        }
        BackendEvent::NewProvider { id } => {
            send_provider_status(ctx.as_ref(), sink.as_ref(), &pctx.sync_status).await?;
            let (tx, rx) = oneshot::channel::<i64>();
            ev_sender
                .send(Event::ProviderSync {
                    id,
                    oneshot: Some(tx),
                })
                .await?;
            let sync_status = pctx.sync_status.clone();
            let sink = sink.clone();
            tokio::spawn(async move {
                if let Ok(id) = rx.await {
                    ctx.oper.wait(id).await;
                    let _ = send_provider_status(ctx.as_ref(), sink.as_ref(), &sync_status).await;
                }
            });
        }
        BackendEvent::UpdateProvider { id } => {
            let _ = id;
            send_provider_status(ctx.as_ref(), sink.as_ref(), &pctx.sync_status).await?;
        }
        BackendEvent::DeleteProvider { id } => {
            let _ = id;
            send_provider_status(ctx.as_ref(), sink.as_ref(), &pctx.sync_status).await?;
        }
        BackendEvent::ReplaceProvider { id } => {
            let _ = id;
            send_provider_status(ctx.as_ref(), sink.as_ref(), &pctx.sync_status).await?;
        }
        BackendEvent::SyncCommit { id, commit } => {
            let status = {
                match pctx.sync_status.get_mut(&id) {
                    Some(v) => {
                        merge_sync_status(v, commit);
                        v.clone()
                    }
                    None => {
                        let mut p = msg::model::ProviderSyncStatus::default();
                        merge_sync_status(&mut p, commit);
                        p.id = id;
                        pctx.sync_status.insert(id, p);
                        p
                    }
                }
            };
            let msg: QcmMessage = msg::ProviderSyncStatusMsg {
                status: Some(status),
                statuses: Vec::new(),
            }
            .qcm_into();
            sink.send_message(msg).await?;
        }
        BackendEvent::End => return Ok(true),
    }
    return Ok(false);
}

fn merge_sync_status(v: &mut msg::model::ProviderSyncStatus, m: SyncCommit) {
    match m {
        SyncCommit::SetState(state) => {
            let id = v.id;
            *v = msg::model::ProviderSyncStatus::default();
            v.id = id;
            v.state = state as i32;
        }
        SyncCommit::AddAlbum(n) => {
            v.album += n;
        }
        SyncCommit::AddArtist(n) => {
            v.artist += n;
        }
        SyncCommit::AddSong(n) => {
            v.song += n;
        }
    }
}

async fn send_provider_meta_status(ctx: &ServiceContext, sink: &dyn EventSink) -> Result<()> {
    let msg: Option<QcmMessage> = {
        let mut msg = ProviderMetaStatusMsg::default();
        global::with_provider_metas(|metas| {
            for (_, v) in metas {
                let m: msg::model::ProviderMeta = v.clone().qcm_into();
                msg.metas.push(m);
            }
        });
        msg.full = true;
        if msg.metas.len() > 0 {
            Some(msg.qcm_into())
        } else {
            None
        }
    };
    if let Some(msg) = msg {
        sink.send_message(msg).await?;
    }
    Ok(())
}

async fn send_provider_status(
    ctx: &ServiceContext,
    sink: &dyn EventSink,
    sync_status: &BTreeMap<i64, msg::model::ProviderSyncStatus>,
) -> Result<Vec<Arc<dyn Provider>>> {
    let db = &ctx.provider_context.db;
    let providers = global::providers();
    let msg: QcmMessage = {
        let mut msg = ProviderStatusMsg::default();

        let libraries = sqlm::library::Entity::find().all(db).await?;

        msg.statuses = providers
            .iter()
            .map(|p| {
                let mut status = ProviderStatus::default();
                status.id = p.id().unwrap_or(-1);
                status.name = p.name();
                status.type_name = p.type_name().to_string();
                status.auth_info = Some(AuthInfo {
                    server_url: p.base_url(),
                    method: p.auth_method().qcm_into(),
                });

                for lib in &libraries {
                    if Some(lib.provider_id) == p.id() {
                        status.libraries.push(lib.clone().qcm_into());
                    }
                }
                if let Some(s) = sync_status.get(&status.id) {
                    status.sync_status = Some(s.clone());
                }
                return status;
            })
            .collect();

        msg.full = true;
        msg.qcm_into()
    };
    sink.send_message(msg).await?;
    Ok(providers)
}
