use std::path::PathBuf;
use std::sync::Arc;

use sea_orm::DatabaseConnection;
use tokio::sync::mpsc::Receiver;

use super::block_store::BlockStore;
use super::io::IoCmd;
use super::io_handler;
use super::reverse::ReverseEvent;

/// Dispatcher: receives NewConnection events and spawns ConnectionHandler tasks.
/// All cache logic is delegated to BlockStore (shared resource).
pub struct Dispatcher {
    rx: Receiver<ReverseEvent>,
    store: Arc<BlockStore>,
    db: DatabaseConnection,
}

impl Dispatcher {
    pub async fn process(
        rx: Receiver<ReverseEvent>,
        db: DatabaseConnection,
        cache_dir: PathBuf,
    ) {
        // Start IO thread
        let (io_tx, io_rx) = std::sync::mpsc::channel::<IoCmd>();
        let io_cache_dir = cache_dir.clone();
        let io_handle = std::thread::spawn(move || {
            io_handler::io_thread(io_rx, io_cache_dir);
        });

        let store = Arc::new(BlockStore::new(io_tx, cache_dir));

        let mut dispatcher = Dispatcher { rx, store, db };
        dispatcher.run().await;

        drop(dispatcher);
        let _ = io_handle.join();
    }

    async fn run(&mut self) {
        while let Some(ev) = self.rx.recv().await {
            match ev {
                ReverseEvent::NewConnection(cnn, ct, rsp_tx) => {
                    let store = self.store.clone();
                    let db = self.db.clone();
                    tokio::spawn(async move {
                        super::connection_handler::ConnectionHandler::process(
                            cnn, store, db, ct, rsp_tx,
                        )
                        .await;
                    });
                }
                ReverseEvent::Stop => break,
            }
        }
    }
}
