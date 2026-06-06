mod api;
mod convert;
mod db;
mod error;
mod event;
mod fts;
mod global;
mod http;
mod msg;
mod reverse;
mod task;

use anyhow;
use clap::{self, Parser};
use fts::load_fts_plugin;
use reverse::ReverseEvent;
use std::{
    collections::HashMap,
    path::{Path, PathBuf},
    time::Duration,
};
use task::TaskManager;

use hyper::{body::Incoming, Request};
use hyper_util::rt::TokioIo;
use tokio::{
    net::{TcpListener, TcpStream},
    sync::watch,
};

use migration::{CacheDBMigrator, Migrator, MigratorTrait};
use sea_orm::{ConnectionTrait, Database, DatabaseConnection};

use api::handler::handle_request;

#[derive(clap::Parser)]
#[command(author, version, about, long_about = None)]
struct Args {
    /// Data directory path
    #[arg(short, long)]
    data: PathBuf,

    /// Cache directory path
    #[arg(short, long)]
    cache: PathBuf,

    /// Port to listen on, auto if not set
    #[arg(short, long)]
    port: Option<u16>,

    /// Log level (error, warn, info, debug, trace)
    #[arg(short, long, env = "RUST_LOG")]
    log_level: Option<String>,
}

fn default_log_filter() -> tracing_subscriber::filter::EnvFilter {
    tracing_subscriber::filter::EnvFilter::builder()
        .with_default_directive(tracing_subscriber::filter::LevelFilter::ERROR.into())
        .parse_lossy("")
}

#[cfg(unix)]
async fn wait_for_signal(tx: watch::Sender<bool>) {
    use tokio::signal::unix::{signal, SignalKind};
    let mut sigterm = signal(SignalKind::terminate()).unwrap();
    let mut sigint = signal(SignalKind::interrupt()).unwrap();
    {
        tokio::select! {
            _ = sigterm.recv() => {}
            _ = sigint.recv() => {}
        };
        tx.send(true).unwrap();
    }
}

#[cfg(windows)]
async fn wait_for_signal(tx: watch::Sender<bool>) {
    {
        tokio::select! {
            _ = tokio::signal::ctrl_c() => {}
        };
        tx.send(true).unwrap();
    }
}

#[derive(Debug, sea_orm::FromQueryResult)]
struct Hello {
    #[sea_orm(column_index = 0)]
    message: String,
}

#[tokio::main]
async fn main() -> Result<(), anyhow::Error> {
    use tracing_subscriber::{filter::EnvFilter, fmt, prelude::*, reload};
    let log_reload_handle = {
        let (filter, reload_handle) = reload::Layer::new(default_log_filter());
        tracing_subscriber::registry()
            .with(filter)
            .with(fmt::Layer::default().with_line_number(true).with_file(true))
            .init();
        reload_handle
    };

    let args = Args::parse();
    let log_level = args
        .log_level
        .and_then(|l| EnvFilter::try_new(&l).ok())
        .unwrap_or(default_log_filter());

    if !load_fts_plugin() {
        panic!("Failed to load fts plugin");
    }

    qcm_core::global::init(&args.data);
    qcm_plugins::init();

    let (oper, taskmgr_handle) = {
        let (oper, mgr) = TaskManager::new();
        (oper, mgr.start())
    };

    // database
    let db = prepare_db(&args.data).await?;
    let cache_db = prepare_cache_db(&args.data).await?;

    // shutdown watcher
    let mut shutdown_rx = {
        let (shutdown_tx, shutdown_rx) = watch::channel(false);
        global::set_shutdown_tx(shutdown_tx.clone());

        tokio::spawn(async move {
            wait_for_signal(shutdown_tx).await;
        });
        shutdown_rx
    };

    let reverse_ev = {
        let cache_db = cache_db.clone();
        let cache_dir = args.cache.clone();
        let (tx, rx) = tokio::sync::mpsc::channel(512);
        tokio::spawn({
            async move {
                reverse::Dispatcher::process(
                    rx,
                    cache_db,
                    cache_dir.join("QcmBackend"),
                )
                .await;
            }
        });

        tx
    };

    // Use port 0 if none specified (system will assign an available port)
    let listener = listen(args.port.unwrap_or(0)).await;

    // enable logging after print port
    let _ = log_reload_handle.modify(|f| {
        *f = log_level;
    });

    // init other gloabl entry here for log
    qcm_core::global::load_from_db(&db).await;

    let accept = {
        let oper = oper.clone();
        let cnn_shutdown_rx = shutdown_rx.clone();
        let reverse_ev = reverse_ev.clone();
        move |stream: TcpStream| {
            let addr = stream
                .peer_addr()
                .expect("connected streams should have a peer address");

            tokio::spawn({
                // need double clone for multiple requests in one connection, `accept` is Fn
                let db = db.clone();
                let cache_db = cache_db.clone();
                let oper = oper.clone();
                let reverse_ev = reverse_ev.clone();
                let mut cnn_shutdown_rx = cnn_shutdown_rx.clone();

                async move {
                    let http = hyper::server::conn::http1::Builder::new();
                    let service = |request: Request<Incoming>| {
                        let db = db.clone();
                        let cache_db = cache_db.clone();
                        let oper = oper.clone();
                        let reverse_ev = reverse_ev.clone();
                        async move { handle_request(request, db, cache_db, oper, reverse_ev).await }
                    };
                    let connection = http
                        .serve_connection(TokioIo::new(stream), hyper::service::service_fn(service))
                        .with_upgrades();

                    log::debug!(target: "http", "start connection: {}", addr);

                    tokio::select! {
                        res = connection => {
                            if let Err(err) = res {
                                log::error!("Error HTTP connection: {err:?}");
                            } else {
                                log::debug!(target: "http", "end connection: {}", addr);
                            }
                        }
                        _ = cnn_shutdown_rx.changed() => {
                            log::info!("shutting down connection: {}", addr);
                        }
                    }
                }
            });
        }
    };

    loop {
        if *shutdown_rx.borrow() {
            break;
        }

        tokio::select! {
            accept_result = listener.accept() => {
                if let Ok((stream, _)) = accept_result {
                    accept(stream);
                }
            }
            _ = shutdown_rx.changed() => {
                log::info!("Shutting down acceptting");
                break;
            }
        }
    }

    // final
    {
        let _ = reverse_ev.send(ReverseEvent::Stop);

        log::info!("Shutting down task manager");
        oper.stop();
        let _ = taskmgr_handle.join();
        log::info!("Task manager stopped");
    }
    Ok(())
}

async fn prepare_db(data: &Path) -> Result<DatabaseConnection, anyhow::Error> {
    let db_path = data.join("backend.2.db");
    let db_url = format!("sqlite://{}?mode=rwc", db_path.to_string_lossy());

    let mut opt = sea_orm::ConnectOptions::new(db_url);

    let enable_logging = std::env::var("QCM_SQL_LOGGING").is_ok();
    opt.sqlx_logging(enable_logging)
        .sqlx_logging_level(log::LevelFilter::Debug)
        .sqlx_slow_statements_logging_settings(log::LevelFilter::Info, Duration::from_secs(1));

    // mmap_size 128MB
    // journal_size_limit 64MB
    // cache_size 8MB
    let db = Database::connect(opt).await?;
    db.execute(sea_orm::Statement::from_string(
        sea_orm::DatabaseBackend::Sqlite,
        "
            PRAGMA foreign_keys = ON;
            PRAGMA journal_mode = WAL;
            PRAGMA synchronous = NORMAL;
            PRAGMA temp_store = MEMORY;
            PRAGMA mmap_size = 134217728;
            PRAGMA journal_size_limit = 67108864;
            PRAGMA cache_size = 2000;
            PRAGMA auto_vacuum = 2;
            PRAGMA busy_timeout = 5000;
            PRAGMA wal_autocheckpoint = 1000;
        "
        .to_owned(),
    ))
    .await?;

    // custom migrator
    Migrator::down(&db, None).await?;
    Migrator::up(&db, None).await?;

    Ok(db)
}

async fn prepare_cache_db(data: &Path) -> Result<DatabaseConnection, anyhow::Error> {
    let db_path = data.join("backend_cache.db");
    let db_url = format!("sqlite://{}?mode=rwc", db_path.to_string_lossy());

    let mut opt = sea_orm::ConnectOptions::new(db_url);

    let enable_logging = std::env::var("QCM_SQL_LOGGING").is_ok();
    opt.sqlx_logging(enable_logging)
        .sqlx_logging_level(log::LevelFilter::Debug)
        .sqlx_slow_statements_logging_settings(log::LevelFilter::Debug, Duration::from_secs(1));

    // mmap_size 128MB
    // journal_size_limit 64MB
    // cache_size 8MB
    let db = Database::connect(opt).await?;
    db.execute(sea_orm::Statement::from_string(
        sea_orm::DatabaseBackend::Sqlite,
        "
            PRAGMA foreign_keys = ON;
            PRAGMA journal_mode = WAL;
            PRAGMA synchronous = NORMAL;
            PRAGMA temp_store = MEMORY;
            PRAGMA mmap_size = 134217728;
            PRAGMA journal_size_limit = 67108864;
            PRAGMA cache_size = 2000;
            PRAGMA auto_vacuum = 2;
            PRAGMA busy_timeout = 5000;
            PRAGMA wal_autocheckpoint = 1000;
        "
        .to_owned(),
    ))
    .await?;

    // custom migrator
    CacheDBMigrator::down(&db, None).await?;
    CacheDBMigrator::up(&db, None).await?;

    Ok(db)
}

async fn listen(port: u16) -> TcpListener {
    let listener = {
        let addr = format!("127.0.0.1:{}", port);

        let try_socket = TcpListener::bind(&addr).await;
        let listener = try_socket.expect("Failed to bind");

        let local_addr = listener.local_addr().expect("Failed to get local address");

        // print port json
        println!(
            "{}",
            serde_json::to_string(&HashMap::from([("port", local_addr.port())])).unwrap()
        );
        listener
    };
    listener
}
