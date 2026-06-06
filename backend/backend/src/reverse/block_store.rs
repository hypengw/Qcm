use bytes::Bytes;
use qcm_core::model as sqlm;
use qcm_core::model::type_enum::CacheType;
use sea_orm::DatabaseConnection;
use std::collections::HashMap;
use std::path::{Path, PathBuf};
use std::sync::{Arc, Mutex, RwLock};
use tokio::sync::Notify;
use tokio::task::JoinHandle;

use super::connection::RemoteFileInfo;
use super::io::IoCmd;

pub const BLOCK_SIZE: u64 = 8 * 1024 * 1024; // 8MB
pub const READ_CHUNK_SIZE: u64 = 64 * 1024; // 64KB per read

/// Block state machine
#[derive(Clone)]
pub enum BlockState {
    /// Being downloaded, partial data available
    Fetching {
        written: u64,
        notify: Arc<Notify>,
    },
    /// Fully cached on disk (and in DB)
    Cached,
}

/// Source-level metadata
#[derive(Clone, Debug)]
pub struct SourceMeta {
    pub content_type: String,
    pub content_length: u64,
    pub accept_ranges: bool,
    pub block_count: u32,
}

impl SourceMeta {
    pub fn from_remote_info(info: &RemoteFileInfo) -> Self {
        let content_length = info.full();
        Self {
            content_type: info.content_type.clone(),
            content_length,
            accept_ranges: info.accept_ranges,
            block_count: block_count(content_length),
        }
    }
}

/// Handle for an active SourceActor
pub struct SourceHandle {
    pub task: JoinHandle<()>,
}

/// Central block-based cache store, shared across all actors via Arc
pub struct BlockStore {
    /// source_key → source metadata
    sources: RwLock<HashMap<String, SourceMeta>>,
    /// block_key → block state (only for non-Cached blocks that are in memory;
    /// Cached blocks may or may not be present — DB is the source of truth)
    blocks: RwLock<HashMap<String, BlockState>>,
    /// source_key → active SourceActor handle
    active_sources: Mutex<HashMap<String, SourceHandle>>,
    /// IO thread command channel
    io_tx: std::sync::mpsc::Sender<IoCmd>,
    /// Cache directory
    cache_dir: PathBuf,
}

// --- Pure functions ---

pub fn block_index(offset: u64) -> u32 {
    (offset / BLOCK_SIZE) as u32
}

pub fn block_key(source_key: &str, index: u32) -> String {
    format!("{}_{}", source_key, index)
}

pub fn block_offset(index: u32) -> u64 {
    index as u64 * BLOCK_SIZE
}

pub fn block_count(content_length: u64) -> u32 {
    ((content_length + BLOCK_SIZE - 1) / BLOCK_SIZE) as u32
}

pub fn block_path(cache_dir: &Path, key: &str) -> PathBuf {
    let prefix = key.get(0..2).unwrap_or("00");
    cache_dir.join(prefix).join(key)
}

pub fn block_path_downloading(cache_dir: &Path, key: &str) -> PathBuf {
    block_path(cache_dir, key).with_extension("downloading")
}

impl BlockStore {
    pub fn new(io_tx: std::sync::mpsc::Sender<IoCmd>, cache_dir: PathBuf) -> Self {
        Self {
            sources: RwLock::new(HashMap::new()),
            blocks: RwLock::new(HashMap::new()),
            active_sources: Mutex::new(HashMap::new()),
            io_tx,
            cache_dir,
        }
    }

    pub fn cache_dir(&self) -> &Path {
        &self.cache_dir
    }

    // --- Source metadata ---

    pub fn get_source_meta(&self, source_key: &str) -> Option<SourceMeta> {
        self.sources.read().unwrap().get(source_key).cloned()
    }

    pub fn set_source_meta(&self, source_key: &str, meta: SourceMeta) {
        self.sources
            .write()
            .unwrap()
            .insert(source_key.to_string(), meta);
    }

    // --- Block state ---

    /// Query block state: memory first, then DB fallback
    pub async fn block_state(&self, bkey: &str, db: &DatabaseConnection) -> Option<BlockState> {
        // Check in-memory state
        {
            let blocks = self.blocks.read().unwrap();
            if let Some(state) = blocks.get(bkey) {
                return Some(state.clone());
            }
        }

        // Check DB
        if sqlm::cache_block::exists(db, bkey).await {
            let state = BlockState::Cached;
            self.blocks
                .write()
                .unwrap()
                .insert(bkey.to_string(), state.clone());
            return Some(state);
        }

        None
    }

    /// Mark a block as being fetched. Returns the Notify handle.
    pub fn begin_fetch(&self, bkey: &str) -> Arc<Notify> {
        let mut blocks = self.blocks.write().unwrap();
        // If already fetching, return existing notify
        if let Some(BlockState::Fetching { notify, .. }) = blocks.get(bkey) {
            return notify.clone();
        }
        let notify = Arc::new(Notify::new());
        blocks.insert(
            bkey.to_string(),
            BlockState::Fetching {
                written: 0,
                notify: notify.clone(),
            },
        );
        notify
    }

    /// Write data to a block (called by SourceActor)
    pub fn write_block_data(&self, bkey: &str, offset_in_block: u64, data: &Bytes) {
        // Send write command to IO thread
        let _ = self.io_tx.send(IoCmd::Write {
            key: bkey.to_string(),
            offset: offset_in_block,
            data: data.clone(),
        });

        // Update in-memory state and notify waiters
        let mut blocks = self.blocks.write().unwrap();
        if let Some(BlockState::Fetching { written, notify }) = blocks.get_mut(bkey) {
            let new_end = offset_in_block + data.len() as u64;
            if new_end > *written {
                *written = new_end;
            }
            notify.notify_waiters();
        }
    }

    /// Mark a block as fully cached
    pub async fn finish_block(
        &self,
        bkey: &str,
        source_key: &str,
        block_index: i32,
        block_size: i64,
        db: &DatabaseConnection,
    ) {
        // Rename .downloading → final
        let from = block_path_downloading(&self.cache_dir, bkey);
        let to = block_path(&self.cache_dir, bkey);
        let _ = self.io_tx.send(IoCmd::Rename {
            key: bkey.to_string(),
            from,
            to,
        });

        // Insert into DB
        use sea_orm::{EntityTrait, Set};
        let model = sqlm::cache_block::ActiveModel {
            key: Set(bkey.to_string()),
            source_key: Set(source_key.to_string()),
            block_index: Set(block_index),
            block_size: Set(block_size),
            blob: sea_orm::NotSet,
            timestamp: sea_orm::NotSet,
        };
        if let Err(e) = sqlm::cache_block::Entity::insert(model)
            .on_conflict(
                sea_orm::sea_query::OnConflict::column(sqlm::cache_block::Column::Key)
                    .update_columns([
                        sqlm::cache_block::Column::BlockSize,
                    ])
                    .to_owned(),
            )
            .exec(db)
            .await
        {
            log::error!(target: "block_store", "finish_block DB error: {:?}", e);
        }

        // Update in-memory state: notify before changing to Cached
        {
            let blocks = self.blocks.read().unwrap();
            if let Some(BlockState::Fetching { notify, .. }) = blocks.get(bkey) {
                notify.notify_waiters();
            }
        }
        self.blocks
            .write()
            .unwrap()
            .insert(bkey.to_string(), BlockState::Cached);
    }

    /// Save source metadata to DB
    pub async fn finish_source(
        &self,
        source_key: &str,
        cache_type: CacheType,
        meta: &SourceMeta,
        db: &DatabaseConnection,
    ) {
        use sea_orm::{EntityTrait, Set};
        let model = sqlm::cache_source::ActiveModel {
            key: Set(source_key.to_string()),
            cache_type: Set(cache_type),
            content_type: Set(meta.content_type.clone()),
            content_length: Set(meta.content_length as i64),
            block_count: Set(meta.block_count as i32),
            timestamp: sea_orm::NotSet,
            last_use: sea_orm::NotSet,
        };
        if let Err(e) = sqlm::cache_source::Entity::insert(model)
            .on_conflict(
                sea_orm::sea_query::OnConflict::column(sqlm::cache_source::Column::Key)
                    .update_columns([
                        sqlm::cache_source::Column::ContentType,
                        sqlm::cache_source::Column::ContentLength,
                        sqlm::cache_source::Column::BlockCount,
                    ])
                    .to_owned(),
            )
            .exec(db)
            .await
        {
            log::error!(target: "block_store", "finish_source DB error: {:?}", e);
        }
    }

    /// Wait until a block has data readable at the given offset
    pub async fn wait_readable(&self, bkey: &str, offset_in_block: u64) -> bool {
        loop {
            let state = {
                let blocks = self.blocks.read().unwrap();
                blocks.get(bkey).cloned()
            };
            match state {
                Some(BlockState::Cached) => return true,
                Some(BlockState::Fetching { written, notify }) => {
                    if written > offset_in_block {
                        return true;
                    }
                    // Wait for new data
                    notify.notified().await;
                }
                None => return false,
            }
        }
    }

    /// Get how many bytes are currently readable in a fetching block
    pub fn readable_bytes(&self, bkey: &str) -> u64 {
        let blocks = self.blocks.read().unwrap();
        match blocks.get(bkey) {
            Some(BlockState::Cached) => BLOCK_SIZE, // full block
            Some(BlockState::Fetching { written, .. }) => *written,
            None => 0,
        }
    }

    /// Read block data via IO thread
    pub async fn read_block(
        &self,
        bkey: &str,
        offset: u64,
        len: u64,
    ) -> Result<Bytes, anyhow::Error> {
        let (tx, rx) = tokio::sync::oneshot::channel();
        self.io_tx.send(IoCmd::Read {
            key: bkey.to_string(),
            offset,
            len,
            reply: tx,
        })?;
        rx.await?
    }

    /// Create a new downloading file for a block
    pub fn create_block_file(&self, bkey: &str) {
        let _ = self.io_tx.send(IoCmd::CreateFile {
            key: bkey.to_string(),
        });
    }

    // --- Source actor management ---

    /// Check if a source needs a new SourceActor. Returns true if no active actor exists.
    pub fn needs_source_actor(&self, source_key: &str) -> bool {
        let sources = self.active_sources.lock().unwrap();
        !sources.contains_key(source_key)
    }

    /// Register a SourceActor task handle
    pub fn set_source_actor(&self, source_key: &str, task: JoinHandle<()>) {
        self.active_sources.lock().unwrap().insert(
            source_key.to_string(),
            SourceHandle { task },
        );
    }

    /// Remove a SourceActor registration
    pub fn remove_source_actor(&self, source_key: &str) {
        self.active_sources
            .lock()
            .unwrap()
            .remove(source_key);
    }

    /// Check if a source has an active SourceActor
    pub fn has_source_actor(&self, source_key: &str) -> bool {
        self.active_sources
            .lock()
            .unwrap()
            .contains_key(source_key)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    // --- 纯函数：block 边界计算 ---

    #[test]
    fn test_block_index_boundaries() {
        assert_eq!(block_index(0), 0);
        assert_eq!(block_index(BLOCK_SIZE - 1), 0); // 8MB-1 仍在 block 0
        assert_eq!(block_index(BLOCK_SIZE), 1);      // 8MB 进入 block 1
        assert_eq!(block_index(BLOCK_SIZE * 3 + 100), 3);
    }

    #[test]
    fn test_block_offset() {
        assert_eq!(block_offset(0), 0);
        assert_eq!(block_offset(1), BLOCK_SIZE);
        assert_eq!(block_offset(5), 5 * BLOCK_SIZE);
    }

    #[test]
    fn test_block_count() {
        assert_eq!(block_count(0), 0);
        assert_eq!(block_count(1), 1);              // 1 byte → 1 block
        assert_eq!(block_count(BLOCK_SIZE), 1);     // 恰好 8MB → 1 block
        assert_eq!(block_count(BLOCK_SIZE + 1), 2); // 8MB+1 → 2 blocks
    }

    #[test]
    fn test_block_key_deterministic() {
        // 相同输入必须产生相同 key（缓存可复现性）
        assert_eq!(block_key("abc123", 0), "abc123_0");
        assert_eq!(block_key("abc123", 42), "abc123_42");
    }

    #[test]
    fn test_block_path_structure() {
        let dir = PathBuf::from("/cache");
        // 取 key 前 2 字符做子目录
        assert_eq!(block_path(&dir, "ab1234_0"), PathBuf::from("/cache/ab/ab1234_0"));
        assert_eq!(
            block_path_downloading(&dir, "ab1234_0"),
            PathBuf::from("/cache/ab/ab1234_0.downloading")
        );
    }

    // --- 状态机：Missing → Fetching → Cached ---

    /// 构造一个不连接真实 IO 线程的 BlockStore（测试用）
    fn test_store() -> Arc<BlockStore> {
        let (io_tx, _io_rx) = std::sync::mpsc::channel();
        Arc::new(BlockStore::new(io_tx, PathBuf::from("/tmp/test_cache")))
    }

    #[test]
    fn test_begin_fetch_creates_fetching_state() {
        let store = test_store();
        let notify = store.begin_fetch("key_0");
        // 状态应为 Fetching，written=0
        assert_eq!(store.readable_bytes("key_0"), 0);
        drop(notify);
    }

    #[test]
    fn test_begin_fetch_idempotent() {
        // 对同一 key 多次 begin_fetch 应返回同一个 Notify
        let store = test_store();
        let n1 = store.begin_fetch("key_0");
        let n2 = store.begin_fetch("key_0");
        // 两次返回的 Notify 是同一个 Arc
        assert!(Arc::ptr_eq(&n1, &n2));
    }

    #[test]
    fn test_write_block_data_updates_written() {
        let store = test_store();
        store.begin_fetch("key_0");

        let data = Bytes::from(vec![0u8; 1024]);
        store.write_block_data("key_0", 0, &data);
        assert_eq!(store.readable_bytes("key_0"), 1024);

        // 追加写入
        store.write_block_data("key_0", 1024, &data);
        assert_eq!(store.readable_bytes("key_0"), 2048);
    }

    #[test]
    fn test_readable_bytes_missing_block() {
        let store = test_store();
        assert_eq!(store.readable_bytes("nonexistent"), 0);
    }

    #[tokio::test]
    async fn test_wait_readable_returns_immediately_when_data_available() {
        let store = test_store();
        store.begin_fetch("key_0");
        store.write_block_data("key_0", 0, &Bytes::from(vec![0u8; 4096]));

        // offset 0 有 4096 字节可读，应立即返回
        let result = store.wait_readable("key_0", 0).await;
        assert!(result);
    }

    #[tokio::test]
    async fn test_wait_readable_returns_false_for_missing() {
        let store = test_store();
        // 未 begin_fetch 的 block → 返回 false
        let result = store.wait_readable("nonexistent", 0).await;
        assert!(!result);
    }

    #[tokio::test]
    async fn test_wait_readable_woken_by_write() {
        // 验证写入唤醒等待中的读取者
        let store = test_store();
        store.begin_fetch("key_0");

        let store2 = store.clone();
        let writer = tokio::spawn(async move {
            // 延迟写入，让 reader 先进入等待
            tokio::time::sleep(std::time::Duration::from_millis(50)).await;
            store2.write_block_data("key_0", 0, &Bytes::from(vec![1u8; 100]));
        });

        // reader 等待 offset 0 的数据
        let readable = store.wait_readable("key_0", 0).await;
        assert!(readable);
        writer.await.unwrap();
    }

    #[tokio::test]
    async fn test_wait_readable_cached_returns_immediately() {
        let store = test_store();
        // 直接设置为 Cached
        store
            .blocks
            .write()
            .unwrap()
            .insert("key_0".to_string(), BlockState::Cached);

        let result = store.wait_readable("key_0", BLOCK_SIZE - 1).await;
        assert!(result); // Cached 状态不管 offset 都返回 true
    }

    // --- Source actor 管理 ---

    #[test]
    fn test_source_actor_lifecycle() {
        let store = test_store();
        assert!(store.needs_source_actor("src1"));

        // 注册后不再需要
        let handle = tokio::runtime::Runtime::new().unwrap().spawn(async {});
        store.set_source_actor("src1", handle);
        assert!(!store.needs_source_actor("src1"));
        assert!(store.has_source_actor("src1"));

        // 移除后恢复
        store.remove_source_actor("src1");
        assert!(store.needs_source_actor("src1"));
    }

    // --- Source metadata ---

    #[test]
    fn test_source_meta_roundtrip() {
        let store = test_store();
        assert!(store.get_source_meta("src1").is_none());

        let meta = SourceMeta {
            content_type: "audio/flac".into(),
            content_length: 20_000_000,
            accept_ranges: true,
            block_count: 3,
        };
        store.set_source_meta("src1", meta.clone());

        let got = store.get_source_meta("src1").unwrap();
        assert_eq!(got.content_length, 20_000_000);
        assert_eq!(got.block_count, 3);
    }
}
