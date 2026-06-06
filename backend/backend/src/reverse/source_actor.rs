use std::sync::Arc;

use bytes::Bytes;
use futures::StreamExt;
use qcm_core::model::type_enum::CacheType;
use sea_orm::DatabaseConnection;

use super::block_store::{self, BlockStore, SourceMeta, BLOCK_SIZE};

/// SourceActor: manages a single upstream download for a source.
/// Streams the response body and splits it into fixed-size blocks,
/// writing each block to the BlockStore.
pub async fn source_actor(
    store: Arc<BlockStore>,
    db: DatabaseConnection,
    source_key: String,
    cache_type: CacheType,
    meta: SourceMeta,
    response: reqwest::Response,
    start_block: u32,
) {
    let mut stream = response.bytes_stream();
    let mut current_block = start_block;
    let mut offset_in_block: u64 = 0;

    // Create the first block file
    let first_key = block_store::block_key(&source_key, current_block);
    store.create_block_file(&first_key);
    store.begin_fetch(&first_key);

    while let Some(chunk) = stream.next().await {
        match chunk {
            Ok(chunk) => {
                process_chunk(
                    &store,
                    &db,
                    &source_key,
                    cache_type,
                    &meta,
                    &chunk,
                    &mut current_block,
                    &mut offset_in_block,
                )
                .await;
            }
            Err(e) => {
                log::error!(target: "source_actor", "stream error for {}: {:?}", source_key, e);
                break;
            }
        }
    }

    // Finish the last block if it has data
    if offset_in_block > 0 {
        let bkey = block_store::block_key(&source_key, current_block);
        store
            .finish_block(
                &bkey,
                &source_key,
                current_block as i32,
                offset_in_block as i64,
                &db,
            )
            .await;
    }

    // Save source metadata to DB
    store
        .finish_source(&source_key, cache_type, &meta, &db)
        .await;

    // Unregister
    store.remove_source_actor(&source_key);
    log::debug!(target: "source_actor", "finished for {}", source_key);
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::path::PathBuf;

    /// 构造测试用 BlockStore（不连接真实 IO 线程）
    fn test_store() -> Arc<BlockStore> {
        let (io_tx, _) = std::sync::mpsc::channel();
        Arc::new(BlockStore::new(io_tx, PathBuf::from("/tmp/test_source")))
    }

    fn test_meta(content_length: u64) -> SourceMeta {
        SourceMeta {
            content_type: "audio/flac".into(),
            content_length,
            accept_ranges: true,
            block_count: block_store::block_count(content_length),
        }
    }

    // --- process_chunk 拆分逻辑 ---

    #[tokio::test]
    async fn test_chunk_within_single_block() {
        // 小于 8MB 的 chunk 不触发 block 切换
        let store = test_store();
        let db = sea_orm::Database::connect("sqlite::memory:").await.unwrap();

        let meta = test_meta(1_000_000);
        let mut block = 0u32;
        let mut offset = 0u64;

        store.begin_fetch(&block_store::block_key("src", 0));

        let chunk = Bytes::from(vec![0xABu8; 4096]);
        process_chunk(&store, &db, "src", CacheType::Audio, &meta, &chunk, &mut block, &mut offset).await;

        assert_eq!(block, 0);      // 没有切换 block
        assert_eq!(offset, 4096);  // offset 前进
        assert_eq!(store.readable_bytes(&block_store::block_key("src", 0)), 4096);
    }

    #[tokio::test]
    async fn test_chunk_exactly_fills_block() {
        // chunk 恰好填满一个 block → 切换到下一个
        let store = test_store();
        let db = sea_orm::Database::connect("sqlite::memory:").await.unwrap();

        let meta = test_meta(BLOCK_SIZE * 2);
        let mut block = 0u32;
        let mut offset = 0u64;

        store.begin_fetch(&block_store::block_key("src", 0));

        let chunk = Bytes::from(vec![0u8; BLOCK_SIZE as usize]);
        process_chunk(&store, &db, "src", CacheType::Audio, &meta, &chunk, &mut block, &mut offset).await;

        // block 应切换到 1，offset 归零
        assert_eq!(block, 1);
        assert_eq!(offset, 0);
    }

    #[tokio::test]
    async fn test_chunk_spans_block_boundary() {
        // chunk 跨越 block 边界 → 拆分写入两个 block
        let store = test_store();
        let db = sea_orm::Database::connect("sqlite::memory:").await.unwrap();

        let meta = test_meta(BLOCK_SIZE * 2);
        let mut block = 0u32;
        let mut offset = BLOCK_SIZE - 100; // 距离 block 边界还有 100 字节

        store.begin_fetch(&block_store::block_key("src", 0));

        // 写入 300 字节：100 进 block 0，200 进 block 1
        let chunk = Bytes::from(vec![0xFFu8; 300]);
        process_chunk(&store, &db, "src", CacheType::Audio, &meta, &chunk, &mut block, &mut offset).await;

        assert_eq!(block, 1);
        assert_eq!(offset, 200);
        // block 1 应有 200 字节
        assert_eq!(store.readable_bytes(&block_store::block_key("src", 1)), 200);
    }

    #[tokio::test]
    async fn test_multiple_small_chunks_accumulate() {
        // 多个小 chunk 逐步累积
        let store = test_store();
        let db = sea_orm::Database::connect("sqlite::memory:").await.unwrap();

        let meta = test_meta(10000);
        let mut block = 0u32;
        let mut offset = 0u64;
        let bkey = block_store::block_key("src", 0);
        store.begin_fetch(&bkey);

        for i in 0..10 {
            let chunk = Bytes::from(vec![i as u8; 1000]);
            process_chunk(&store, &db, "src", CacheType::Audio, &meta, &chunk, &mut block, &mut offset).await;
        }

        assert_eq!(block, 0);       // 10KB 不足以填满 8MB block
        assert_eq!(offset, 10000);
        assert_eq!(store.readable_bytes(&bkey), 10000);
    }
}

/// 将一个 chunk 按 block 边界拆分写入。
/// 跨 block 边界时自动 finish 当前 block 并开始下一个。
async fn process_chunk(
    store: &BlockStore,
    db: &DatabaseConnection,
    source_key: &str,
    cache_type: CacheType,
    meta: &SourceMeta,
    chunk: &Bytes,
    current_block: &mut u32,
    offset_in_block: &mut u64,
) {
    let mut remaining = chunk.as_ref();

    while !remaining.is_empty() {
        let block_remaining = BLOCK_SIZE - *offset_in_block;
        let take = remaining.len().min(block_remaining as usize);
        let data = Bytes::copy_from_slice(&remaining[..take]);
        let bkey = block_store::block_key(source_key, *current_block);

        // Write to current block
        store.write_block_data(&bkey, *offset_in_block, &data);

        *offset_in_block += take as u64;
        remaining = &remaining[take..];

        // Block is full — finish it and start next
        if *offset_in_block >= BLOCK_SIZE {
            store
                .finish_block(
                    &bkey,
                    source_key,
                    *current_block as i32,
                    BLOCK_SIZE as i64,
                    db,
                )
                .await;

            *current_block += 1;
            *offset_in_block = 0;

            // Prepare next block
            let next_key = block_store::block_key(source_key, *current_block);
            store.create_block_file(&next_key);
            store.begin_fetch(&next_key);
        }
    }
}
