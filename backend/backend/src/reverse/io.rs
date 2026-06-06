use bytes::Bytes;
use std::path::PathBuf;

/// Commands sent to the IO thread for blocking file operations
pub enum IoCmd {
    /// Read data from a cached block file
    Read {
        key: String,
        offset: u64,
        len: u64,
        reply: tokio::sync::oneshot::Sender<Result<Bytes, anyhow::Error>>,
    },
    /// Write data to a downloading block file
    Write {
        key: String,
        offset: u64,
        data: Bytes,
    },
    /// Create a new downloading file for a block
    CreateFile {
        key: String,
    },
    /// Rename a downloading file to its final name
    Rename {
        key: String,
        from: PathBuf,
        to: PathBuf,
    },
}
