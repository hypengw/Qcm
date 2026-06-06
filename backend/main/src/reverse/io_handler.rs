use super::block_store::{block_path, block_path_downloading};
use super::io::IoCmd;
use bytes::{BufMut, Bytes};
use std::collections::HashMap;
use std::fs::{self, File, OpenOptions};
use std::io::{Read, Seek, SeekFrom, Write};
use std::path::{Path, PathBuf};

/// Open file handle cache keyed by cache_key
struct FileCache {
    handles: HashMap<String, File>,
    capacity: usize,
}

impl FileCache {
    fn new(capacity: usize) -> Self {
        Self {
            handles: HashMap::new(),
            capacity,
        }
    }

    fn evict_one(&mut self) {
        if self.handles.len() >= self.capacity {
            if let Some(first_key) = self.handles.keys().next().cloned() {
                self.handles.remove(&first_key);
            }
        }
    }

    /// Open or reuse a file handle with read+write, no truncation.
    fn get_or_open_rw(&mut self, key: &str, path: &Path) -> std::io::Result<&mut File> {
        if !self.handles.contains_key(key) {
            self.evict_one();
            if let Some(parent) = path.parent() {
                fs::create_dir_all(parent)?;
            }
            let file = OpenOptions::new()
                .read(true)
                .write(true)
                .create(true)
                .open(path)?;
            self.handles.insert(key.to_string(), file);
        }
        Ok(self.handles.get_mut(key).unwrap())
    }

    /// Open or reuse a file handle for reading only.
    fn get_or_open_read(&mut self, key: &str, path: &Path) -> std::io::Result<&mut File> {
        if !self.handles.contains_key(key) {
            self.evict_one();
            let file = File::open(path)?;
            self.handles.insert(key.to_string(), file);
        }
        Ok(self.handles.get_mut(key).unwrap())
    }

    fn remove(&mut self, key: &str) {
        self.handles.remove(key);
    }
}

/// IO thread entry point — pure command executor, no business logic
pub fn io_thread(rx: std::sync::mpsc::Receiver<IoCmd>, cache_dir: PathBuf) {
    let mut file_cache = FileCache::new(128);

    while let Ok(cmd) = rx.recv() {
        match cmd {
            IoCmd::Read {
                key,
                offset,
                len,
                reply,
            } => {
                let result = read_block_sync(&mut file_cache, &cache_dir, &key, offset, len);
                let _ = reply.send(result);
            }
            IoCmd::Write { key, offset, data } => {
                if let Err(e) =
                    write_block_sync(&mut file_cache, &cache_dir, &key, offset, &data)
                {
                    log::error!(target: "io", "write error for {}: {:?}", key, e);
                }
            }
            IoCmd::CreateFile { key } => {
                let path = block_path_downloading(&cache_dir, &key);
                if let Some(parent) = path.parent() {
                    let _ = fs::create_dir_all(parent);
                }
                let cache_key = format!("{}.dl", key);
                match OpenOptions::new()
                    .read(true)
                    .write(true)
                    .create(true)
                    .truncate(true)
                    .open(&path)
                {
                    Ok(file) => {
                        file_cache.handles.insert(cache_key, file);
                    }
                    Err(e) => {
                        log::error!(target: "io", "create file error for {}: {:?}", key, e);
                    }
                }
            }
            IoCmd::Rename { key, from, to } => {
                // Close handles before rename
                file_cache.remove(&format!("{}.dl", key));
                file_cache.remove(&key);
                if let Err(e) = fs::rename(&from, &to) {
                    log::error!(target: "io", "rename error {:?} -> {:?}: {:?}", from, to, e);
                }
            }
        }
    }
}

fn read_block_sync(
    file_cache: &mut FileCache,
    cache_dir: &Path,
    key: &str,
    offset: u64,
    len: u64,
) -> Result<Bytes, anyhow::Error> {
    let final_path = block_path(cache_dir, key);
    let dl_path = block_path_downloading(cache_dir, key);

    // For downloading files, use the rw handle (same fd that writes use).
    // For finished files, open read-only.
    let dl_cache_key = format!("{}.dl", key);
    let file = if file_cache.handles.contains_key(&dl_cache_key) {
        // Reuse the rw handle that CreateFile/Write opened — flush first
        let f = file_cache.handles.get_mut(&dl_cache_key).unwrap();
        f.flush()?;
        f
    } else if final_path.exists() {
        file_cache.get_or_open_read(key, &final_path)?
    } else if dl_path.exists() {
        // Downloading file exists but no cached handle — open rw
        file_cache.get_or_open_rw(&dl_cache_key, &dl_path)?
    } else {
        return Err(anyhow::anyhow!("block file not found: {}", key));
    };

    file.seek(SeekFrom::Start(offset))?;

    let read_len = len.min(64 * 1024) as usize;
    let mut buf = vec![0u8; read_len];
    let n = file.read(&mut buf)?;
    buf.truncate(n);

    let mut bytes_buf = bytes::BytesMut::with_capacity(n);
    bytes_buf.put(&buf[..]);
    Ok(bytes_buf.freeze())
}

fn write_block_sync(
    file_cache: &mut FileCache,
    cache_dir: &Path,
    key: &str,
    offset: u64,
    data: &[u8],
) -> std::io::Result<()> {
    let path = block_path_downloading(cache_dir, key);
    let cache_key = format!("{}.dl", key);
    let file = file_cache.get_or_open_rw(&cache_key, &path)?;
    file.seek(SeekFrom::Start(offset))?;
    file.write_all(data)?;
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    /// 创建临时目录用于测试
    fn temp_cache_dir(name: &str) -> PathBuf {
        let dir = std::env::temp_dir().join(format!("qcm_test_{}", name));
        let _ = fs::remove_dir_all(&dir);
        fs::create_dir_all(&dir).unwrap();
        dir
    }

    // --- 基本读写 ---

    #[test]
    fn test_create_write_read_same_block() {
        // 模拟 CreateFile → Write → Read 的顺序，验证 rw 句柄复用无 EBADF
        let dir = temp_cache_dir("create_write_read");
        let mut fc = FileCache::new(128);
        let key = "aabb_0";

        // 模拟 CreateFile
        let path = block_path_downloading(&dir, key);
        fs::create_dir_all(path.parent().unwrap()).unwrap();
        let file = std::fs::OpenOptions::new()
            .read(true).write(true).create(true).truncate(true)
            .open(&path).unwrap();
        fc.handles.insert(format!("{}.dl", key), file);

        // Write
        write_block_sync(&mut fc, &dir, key, 0, b"hello world").unwrap();
        // Read — 复用同一句柄，不应 EBADF
        let data = read_block_sync(&mut fc, &dir, key, 0, 11).unwrap();
        assert_eq!(&data[..], b"hello world");

        let _ = fs::remove_dir_all(&dir);
    }

    #[test]
    fn test_write_then_read_via_file_cache() {
        // 直接测试 write_block_sync + read_block_sync，绕过线程
        let dir = temp_cache_dir("write_read_fc");
        let mut fc = FileCache::new(128);

        // 写入 downloading 文件
        write_block_sync(&mut fc, &dir, "cc00_0", 0, b"abcdefgh").unwrap();
        write_block_sync(&mut fc, &dir, "cc00_0", 8, b"ijklmnop").unwrap();

        // 从同一 downloading 文件读取（rw 句柄复用）
        let data = read_block_sync(&mut fc, &dir, "cc00_0", 0, 16).unwrap();
        assert_eq!(&data[..], b"abcdefghijklmnop");

        // 从 offset 中间读取
        let data = read_block_sync(&mut fc, &dir, "cc00_0", 4, 8).unwrap();
        assert_eq!(&data[..], b"efghijkl");

        let _ = fs::remove_dir_all(&dir);
    }

    #[test]
    fn test_rename_then_read_final_path() {
        // 写入 .downloading → rename → 从 final path 读取
        let dir = temp_cache_dir("rename_read");
        let mut fc = FileCache::new(128);

        write_block_sync(&mut fc, &dir, "dd00_0", 0, b"test data").unwrap();

        // rename: 关闭旧句柄，重命名文件
        let from = block_path_downloading(&dir, "dd00_0");
        let to = block_path(&dir, "dd00_0");
        fc.remove("dd00_0.dl");
        fs::rename(&from, &to).unwrap();

        // 从 final path 读取（会打开新的只读句柄）
        let data = read_block_sync(&mut fc, &dir, "dd00_0", 0, 9).unwrap();
        assert_eq!(&data[..], b"test data");

        let _ = fs::remove_dir_all(&dir);
    }

    #[test]
    fn test_lru_eviction_no_truncate() {
        // LRU 驱逐后重新打开不应 truncate 已有数据
        let dir = temp_cache_dir("lru_evict");
        let mut fc = FileCache::new(2); // 极小容量，强制驱逐

        // 写入 key A
        write_block_sync(&mut fc, &dir, "aa00_0", 0, b"data_a").unwrap();
        // 写入 key B 和 C，驱逐 A 的句柄
        write_block_sync(&mut fc, &dir, "bb00_0", 0, b"data_b").unwrap();
        write_block_sync(&mut fc, &dir, "cc00_0", 0, b"data_c").unwrap();

        // A 被驱逐，重新打开不应 truncate
        let data = read_block_sync(&mut fc, &dir, "aa00_0", 0, 6).unwrap();
        assert_eq!(&data[..], b"data_a");

        let _ = fs::remove_dir_all(&dir);
    }

    #[test]
    fn test_read_nonexistent_block() {
        let dir = temp_cache_dir("read_noexist");
        let mut fc = FileCache::new(128);

        let result = read_block_sync(&mut fc, &dir, "zz00_0", 0, 10);
        assert!(result.is_err());

        let _ = fs::remove_dir_all(&dir);
    }

    #[test]
    fn test_write_at_offset() {
        // 在非零 offset 写入，验证 seek 正确
        let dir = temp_cache_dir("write_offset");
        let mut fc = FileCache::new(128);

        // 先写 offset 0
        write_block_sync(&mut fc, &dir, "ee00_0", 0, b"\x00\x00\x00\x00").unwrap();
        // 再写 offset 4
        write_block_sync(&mut fc, &dir, "ee00_0", 4, b"\xff\xff").unwrap();

        let data = read_block_sync(&mut fc, &dir, "ee00_0", 0, 6).unwrap();
        assert_eq!(&data[..], b"\x00\x00\x00\x00\xff\xff");

        let _ = fs::remove_dir_all(&dir);
    }
}
