use std::sync::Arc;

use bytes::Bytes;
use futures::channel::mpsc::Sender as BoundedSender;
use futures_util::SinkExt;
use hyper::body::Frame;
use sea_orm::DatabaseConnection;

use super::block_store::{self, BlockState, BlockStore, SourceMeta, BLOCK_SIZE, READ_CHUNK_SIZE};
use super::connection::{
    create_rsp, real_request, Connection, Creator, RemoteFileInfo, ResponceOneshot,
};
use super::source_actor;
use crate::error::ProcessError;
use crate::http::body_type::StreamItem;
use qcm_core::model::type_enum::CacheType;

/// Simplified connection handler using block-based cache.
/// Maps the client request to a sequence of blocks and streams them.
pub struct ConnectionHandler;

impl ConnectionHandler {
    pub async fn process(
        cnn: Connection,
        store: Arc<BlockStore>,
        db: DatabaseConnection,
        ct: Creator,
        rsp_tx: ResponceOneshot,
    ) {
        if let Err(e) = Self::process_inner(cnn, store, db, ct, rsp_tx).await {
            log::error!(target: "connection", "error: {:?}", e);
        }
    }

    async fn process_inner(
        cnn: Connection,
        store: Arc<BlockStore>,
        db: DatabaseConnection,
        ct: Creator,
        rsp_tx: ResponceOneshot,
    ) -> Result<(), ProcessError> {
        let source_key = &cnn.key;

        // 1. Get source metadata (from memory, DB, or upstream)
        let (source_meta, maybe_response) =
            get_source_meta(source_key, &store, &db, &ct, &cnn).await?;

        // 2. Compute byte range
        let content_length = source_meta.content_length;
        let (start, serve_length) = match &cnn.range {
            Some(r) => {
                let s = r.start(content_length);
                (s, content_length - s)
            }
            None => (0, content_length),
        };

        // 3. Build RemoteFileInfo for response headers
        let file_info = RemoteFileInfo {
            content_type: source_meta.content_type.clone(),
            content_length: serve_length,
            accept_ranges: source_meta.accept_ranges,
            content_range: cnn.range.as_ref().and_then(|r| {
                crate::http::range::HttpContentRange::from_range(r.clone(), content_length)
            }),
        };

        // 4. Send HTTP response headers
        let (rsp, stream_tx) = create_rsp(0, &cnn.range, &file_info);
        if rsp_tx.send(Ok(rsp)).is_err() {
            return Ok(()); // Client disconnected
        }

        // 5. Ensure SourceActor if we got a response body (cache miss)
        if let Some(response) = maybe_response {
            let start_block = block_store::block_index(start);
            if store.needs_source_actor(source_key) {
                let store2 = store.clone();
                let db2 = db.clone();
                let sk = source_key.to_string();
                let meta2 = source_meta.clone();
                let ct = cnn.cache_type;
                let handle = tokio::spawn(async move {
                    source_actor::source_actor(
                        store2, db2, sk, ct, meta2, response, start_block,
                    )
                    .await;
                });
                store.set_source_actor(source_key, handle);
            }
        }

        // 6. Stream blocks to client
        serve_blocks(&store, &db, source_key, &source_meta, &cnn, start, serve_length, stream_tx)
            .await?;

        Ok(())
    }
}

/// Get source metadata, fetching from upstream if necessary.
/// Returns the metadata and optionally the response body (for cache miss).
async fn get_source_meta(
    source_key: &str,
    store: &BlockStore,
    db: &DatabaseConnection,
    ct: &Creator,
    cnn: &Connection,
) -> Result<(SourceMeta, Option<reqwest::Response>), ProcessError> {
    // Try memory cache
    if let Some(meta) = store.get_source_meta(source_key) {
        return Ok((meta, None));
    }

    // Try DB
    if let Some(db_source) = qcm_core::model::cache_source::query_by_key(db, source_key).await {
        let meta = SourceMeta {
            content_type: db_source.content_type,
            content_length: db_source.content_length as u64,
            accept_ranges: true,
            block_count: db_source.block_count as u32,
        };
        store.set_source_meta(source_key, meta.clone());
        return Ok((meta, None));
    }

    // Fetch from upstream (single GET, extract metadata from headers)
    let (info, response) = real_request(ct, cnn.range.clone()).await?;
    let meta = SourceMeta::from_remote_info(&info);
    store.set_source_meta(source_key, meta.clone());
    Ok((meta, Some(response)))
}

/// Stream block data to the client
async fn serve_blocks(
    store: &BlockStore,
    db: &DatabaseConnection,
    source_key: &str,
    source_meta: &SourceMeta,
    cnn: &Connection,
    start: u64,
    serve_length: u64,
    mut stream_tx: BoundedSender<StreamItem>,
) -> Result<(), ProcessError> {
    let end = start + serve_length;
    let mut cursor = start;

    while cursor < end {
        let block_idx = block_store::block_index(cursor);
        let bkey = block_store::block_key(source_key, block_idx);
        let block_start = block_store::block_offset(block_idx);
        let block_end = (block_start + BLOCK_SIZE).min(source_meta.content_length);
        let offset_in_block = cursor - block_start;

        // Determine block state
        let state = store.block_state(&bkey, db).await;

        match state {
            Some(BlockState::Cached) => {
                // Read from fully cached block
                cursor = serve_cached_block(
                    store,
                    &bkey,
                    offset_in_block,
                    block_end,
                    cursor,
                    end,
                    &mut stream_tx,
                )
                .await?;
            }
            Some(BlockState::Fetching { .. }) => {
                // Read from block being downloaded
                cursor = serve_fetching_block(
                    store,
                    &bkey,
                    offset_in_block,
                    block_end,
                    cursor,
                    end,
                    &mut stream_tx,
                )
                .await?;
            }
            None => {
                // Block missing — ensure SourceActor is running
                if !store.has_source_actor(source_key) {
                    // No active download — we need to trigger one
                    // This shouldn't normally happen if SourceActor was started above,
                    // but handle it for robustness (e.g., seek to uncached block)
                    log::warn!(target: "connection", "block {} missing and no source actor", bkey);
                    return Err(ProcessError::NotFound);
                }

                // Begin fetch tracking and wait
                store.begin_fetch(&bkey);
                cursor = serve_fetching_block(
                    store,
                    &bkey,
                    offset_in_block,
                    block_end,
                    cursor,
                    end,
                    &mut stream_tx,
                )
                .await?;
            }
        }
    }

    Ok(())
}

/// Serve data from a fully cached block, reading in chunks
async fn serve_cached_block(
    store: &BlockStore,
    bkey: &str,
    mut offset_in_block: u64,
    block_end: u64,
    mut cursor: u64,
    end: u64,
    stream_tx: &mut BoundedSender<StreamItem>,
) -> Result<u64, ProcessError> {
    while cursor < end && cursor < block_end {
        let remaining_in_block = block_end - cursor;
        let remaining_in_request = end - cursor;
        let read_len = remaining_in_block
            .min(remaining_in_request)
            .min(READ_CHUNK_SIZE);

        let data = store
            .read_block(bkey, offset_in_block, read_len)
            .await
            .map_err(|e| ProcessError::Internal(e.into()))?;

        if data.is_empty() {
            break;
        }

        let n = data.len() as u64;
        if stream_tx.send(Ok(Frame::data(data))).await.is_err() {
            return Err(ProcessError::NotFound); // Client disconnected
        }

        cursor += n;
        offset_in_block += n;
    }
    Ok(cursor)
}

/// Serve data from a block that is still being downloaded
async fn serve_fetching_block(
    store: &BlockStore,
    bkey: &str,
    mut offset_in_block: u64,
    block_end: u64,
    mut cursor: u64,
    end: u64,
    stream_tx: &mut BoundedSender<StreamItem>,
) -> Result<u64, ProcessError> {
    while cursor < end && cursor < block_end {
        // Wait until data is available at our offset
        if !store.wait_readable(bkey, offset_in_block).await {
            return Err(ProcessError::Internal(anyhow::anyhow!(
                "block {} became unavailable",
                bkey
            )));
        }

        // Determine how much we can read
        let readable = store.readable_bytes(bkey);
        let available = if readable > offset_in_block {
            readable - offset_in_block
        } else {
            // Block transitioned to Cached — read what we need
            block_end - cursor
        };

        let remaining_in_request = end - cursor;
        let read_len = available
            .min(remaining_in_request)
            .min(READ_CHUNK_SIZE);

        if read_len == 0 {
            // Need to wait more
            continue;
        }

        let data = store
            .read_block(bkey, offset_in_block, read_len)
            .await
            .map_err(|e| ProcessError::Internal(e.into()))?;

        if data.is_empty() {
            // IO returned empty — data not yet flushed, retry
            tokio::task::yield_now().await;
            continue;
        }

        let n = data.len() as u64;
        if stream_tx.send(Ok(Frame::data(data))).await.is_err() {
            return Err(ProcessError::NotFound);
        }

        cursor += n;
        offset_in_block += n;
    }
    Ok(cursor)
}
