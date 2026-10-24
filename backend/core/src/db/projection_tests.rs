use super::*;
use futures::FutureExt;

#[tokio::test]
async fn projection_snapshot_covers_commit_rollback_and_local_writes() {
    let db = Database::connect("sqlite::memory:").await.unwrap();
    db.execute_unprepared("CREATE TABLE state (id INTEGER PRIMARY KEY, name TEXT, relation INTEGER); INSERT INTO state VALUES(1,'before',1)").await.unwrap();
    let coordinator = SyncCoordinator::default();
    let first = coordinator.read_snapshot().await;
    let initial = first.revision;
    let writer = coordinator.write(7, None);
    tokio::pin!(writer);
    assert!(writer.as_mut().now_or_never().is_none());
    drop(first);
    let write = writer.await.unwrap();
    let txn = db.begin().await.unwrap();
    txn.execute_unprepared("UPDATE state SET name='after'")
        .await
        .unwrap();
    let snapshot = coordinator.read_snapshot();
    tokio::pin!(snapshot);
    assert!(snapshot.as_mut().now_or_never().is_none());
    txn.execute_unprepared("UPDATE state SET relation=2")
        .await
        .unwrap();
    txn.commit().await.unwrap();
    drop(write);
    let snapshot = snapshot.await;
    assert!(snapshot.revision > initial);
    let row = db
        .query_one(Statement::from_string(
            DbBackend::Sqlite,
            "SELECT name,relation FROM state",
        ))
        .await
        .unwrap()
        .unwrap();
    assert_eq!(row.try_get::<String>("", "name").unwrap(), "after");
    assert_eq!(row.try_get::<i32>("", "relation").unwrap(), 2);
    let local = coordinator.write(0, None);
    tokio::pin!(local);
    assert!(local.as_mut().now_or_never().is_none());
    let committed = snapshot.revision;
    drop(snapshot);
    let local = local.await.unwrap();
    let txn = db.begin().await.unwrap();
    txn.execute_unprepared("UPDATE state SET name='rolled back', relation=3")
        .await
        .unwrap();
    txn.rollback().await.unwrap();
    drop(local);
    let snapshot = coordinator.read_snapshot().await;
    assert!(snapshot.revision > committed);
    let row = db
        .query_one(Statement::from_string(
            DbBackend::Sqlite,
            "SELECT name,relation FROM state",
        ))
        .await
        .unwrap()
        .unwrap();
    assert_eq!(row.try_get::<String>("", "name").unwrap(), "after");
    assert_eq!(row.try_get::<i32>("", "relation").unwrap(), 2);
}

#[tokio::test]
async fn projection_epochs_change_under_the_write_barrier() {
    let coordinator = SyncCoordinator::default();
    let write = coordinator.write(7, None).await.unwrap();
    write.invalidate_scope().unwrap();
    let snapshot = coordinator.read_snapshot();
    tokio::pin!(snapshot);
    assert!(snapshot.as_mut().now_or_never().is_none());
    drop(write);
    let snapshot = snapshot.await;
    assert_eq!(snapshot.epochs.get(&7), Some(&1));
    assert_eq!(snapshot.epochs.get(&0), Some(&1));
    let first_instance = snapshot.instance.clone();
    drop(snapshot);
    assert_ne!(
        SyncCoordinator::default().read_snapshot().await.instance,
        first_instance
    );
}
