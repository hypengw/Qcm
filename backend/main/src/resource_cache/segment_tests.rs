use super::*;
struct Directory(PathBuf);
impl Directory {
    fn new() -> Self {
        let path = std::env::temp_dir().join(format!("qcm-segment-{}", uuid::Uuid::new_v4()));
        fs::create_dir(&path).unwrap();
        Self(path)
    }
}
impl Drop for Directory {
    fn drop(&mut self) {
        let _ = fs::remove_dir_all(&self.0);
    }
}
fn metadata(length: u64) -> Metadata {
    Metadata {
        key: "segments".into(),
        provider: 1,
        kind: 1,
        generation: uuid::Uuid::new_v4().simple().to_string(),
        geometry: Geometry::new(length).unwrap(),
        headers: vec![],
        policy: Policy::from_response(&Default::default(), 0, 0),
        target: "test".into(),
        ranges: true,
        layout: Layout::Blocks,
    }
}
fn data(meta: &Metadata, block: u64) -> Bytes {
    let r = meta.geometry.range(block).unwrap();
    Bytes::from(vec![(block % 251) as u8; (r.end - r.start) as usize])
}
#[test]
fn sparse_segments_survive_restart_and_isolate_damage() {
    for length in [format::SEGMENT_BYTES + 17, 5 * 1024 * 1024 * 1024 + 13] {
        let dir = Directory::new();
        let mut store = Storage::open(dir.0.clone()).unwrap();
        let version = store.create(metadata(length)).unwrap();
        let meta = version.meta.clone();
        let boundary = format::SEGMENT_BYTES / meta.geometry.block;
        let tail = meta.geometry.count() - 1;
        let blocks = [boundary - 1, boundary, tail];
        store
            .write(version.id, &blocks.map(|i| (i, data(&meta, i))))
            .unwrap();
        for entry in fs::read_dir(&dir.0).unwrap() {
            let entry = entry.unwrap();
            if entry.path().extension().is_some_and(|s| s == "seg") {
                assert!(entry.metadata().unwrap().len() <= format::SEGMENT_BYTES + HEADER_LEN);
            }
        }
        let before = store.usage().unwrap().audio;
        store
            .write(version.id, &blocks.map(|i| (i, data(&meta, i))))
            .unwrap();
        assert_eq!(before, store.usage().unwrap().audio);
        assert!(before < 4 * meta.geometry.block);
        assert!(store.read(version.id, 0).unwrap().is_none());
        drop(store);
        let mut store = Storage::open(dir.0.clone()).unwrap();
        let version = store.lookup("segments").unwrap().unwrap();
        for i in blocks {
            assert_eq!(store.read(version.id, i).unwrap(), Some(data(&meta, i)));
        }
        let segment = store.segment(version.id, 0).unwrap().unwrap();
        fs::remove_file(dir.0.join(segment.name(&meta))).unwrap();
        assert!(store.read(version.id, boundary - 1).unwrap().is_none());
        assert_eq!(
            store.read(version.id, tail).unwrap(),
            Some(data(&meta, tail))
        );
        store
            .write(version.id, &[(boundary - 1, data(&meta, boundary - 1))])
            .unwrap();
        assert_ne!(
            segment.token,
            store.segment(version.id, 0).unwrap().unwrap().token
        );
        let segment = store.segment(version.id, 1).unwrap().unwrap();
        let mut file = OpenOptions::new()
            .write(true)
            .open(dir.0.join(segment.name(&meta)))
            .unwrap();
        file.seek(SeekFrom::Start(HEADER_LEN)).unwrap();
        file.write_all(&[255]).unwrap();
        assert!(store.read(version.id, boundary).unwrap().is_none());
        store
            .write(version.id, &[(boundary, data(&meta, boundary))])
            .unwrap();
        assert_eq!(
            store.read(version.id, boundary).unwrap(),
            Some(data(&meta, boundary))
        );
        let damaged = store.segment(version.id, 0).unwrap().unwrap();
        let mut file = OpenOptions::new()
            .write(true)
            .open(dir.0.join(damaged.name(&meta)))
            .unwrap();
        file.write_all(b"broken!!").unwrap();
        store.repair_segments(version.id).unwrap();
        assert!(store.read(version.id, boundary - 1).unwrap().is_none());
        assert!(store.read(version.id, boundary).unwrap().is_some());
    }
}
#[test]
fn window_pins_allow_cold_segments_to_be_evicted_with_live_version() {
    let dir = Directory::new();
    let mut store = Storage::open(dir.0.clone()).unwrap();
    let version = store.create(metadata(format::SEGMENT_BYTES * 3)).unwrap();
    let step = format::SEGMENT_BYTES / version.meta.geometry.block;
    for block in [0, step, step * 2] {
        store
            .write(version.id, &[(block, data(&version.meta, block))])
            .unwrap();
    }
    let _window = store.pin_segments(version.id, 1..2);
    let usage = store.usage().unwrap();
    store.capacity = usage.audio + usage.image + usage.metadata + 64 * 1024;
    store.evict(version.meta.geometry.block).unwrap();
    assert!(version.is_valid());
    assert!(store.read(version.id, 0).unwrap().is_none());
    assert!(store.read(version.id, step).unwrap().is_some());
    assert!(store.read(version.id, step * 2).unwrap().is_some());
    store.capacity = 0;
    assert!(store.evict(0).is_err());
    assert!(store.read(version.id, step).unwrap().is_some());
    let id = version.id;
    drop(version);
    store.delete(id).unwrap();
    assert!(store.read(id, step).unwrap().is_some());
    drop(_window);
    store.capacity = 1024 * 1024;
    store.evict(0).unwrap();
}
#[test]
fn failed_cross_segment_batches_never_publish_and_delete_failure_counts_bytes() {
    for fault in [Fault::Write, Fault::Sync, Fault::Sql] {
        let dir = Directory::new();
        let mut store = Storage::open(dir.0.clone()).unwrap();
        let version = store.create(metadata(format::SEGMENT_BYTES * 2)).unwrap();
        let boundary = format::SEGMENT_BYTES / version.meta.geometry.block;
        let blocks = [
            (boundary - 1, data(&version.meta, boundary - 1)),
            (boundary, data(&version.meta, boundary)),
        ];
        store.fault = Some(fault);
        assert!(store.write(version.id, &blocks).is_err());
        for (i, _) in &blocks {
            assert!(store.read(version.id, *i).unwrap().is_none());
        }
        store.write(version.id, &blocks).unwrap();
        let bytes = store.usage().unwrap().audio;
        store.fault = Some(Fault::SegmentDelete);
        assert!(store.remove_segment(version.id, 0).is_err());
        assert_eq!(store.usage().unwrap().audio, bytes);
        assert!(store.read(version.id, boundary - 1).unwrap().is_none());
        assert!(store.read(version.id, boundary).unwrap().is_some());
        drop(store);
        let mut store = Storage::open(dir.0.clone()).unwrap();
        let version = store.lookup("segments").unwrap().unwrap();
        assert!(store.usage().unwrap().audio < bytes);
        assert!(store.read(version.id, boundary - 1).unwrap().is_none());
        assert!(store.read(version.id, boundary).unwrap().is_some());
    }
}
#[test]
fn layouts_and_schema_preserve_legacy_data() {
    assert_eq!(
        Layout::for_resource(1, format::SEGMENT_BYTES),
        Layout::Blocks
    );
    assert_eq!(
        Layout::for_resource(0, format::SEGMENT_BYTES + 1),
        Layout::Blocks
    );
    for length in [0, 17, format::SEGMENT_BYTES, format::SEGMENT_BYTES * 2 + 1] {
        let dir = Directory::new();
        let mut store = Storage::open(dir.0.clone()).unwrap();
        let meta = metadata(length);
        let version = store.create_layout(meta.clone()).unwrap();
        if length > 0 {
            store.write(version.id, &[(0, data(&meta, 0))]).unwrap();
        }
        drop(store);
        let mut store = Storage::open(dir.0.clone()).unwrap();
        let version = store.lookup("segments").unwrap().unwrap();
        assert_eq!(version.meta.layout, Layout::Blocks);
        if length > 0 {
            assert_eq!(store.read(version.id, 0).unwrap(), Some(data(&meta, 0)));
        }
        store.db.exec("PRAGMA user_version=4").unwrap();
        drop(store);
        assert!(Storage::open(dir.0.clone()).is_err());
    }
}
#[test]
#[ignore = "subprocess crash helper"]
fn segment_crash_worker() {
    let Some(root) = std::env::var_os("QCM_SEGMENT_CRASH_ROOT") else {
        return;
    };
    let mut store = Storage::open(PathBuf::from(root)).unwrap();
    let version = store.create(metadata(format::SEGMENT_BYTES * 2)).unwrap();
    let point = std::env::var("QCM_SEGMENT_CRASH_POINT").unwrap();
    store.fault = Some(match point.as_str() {
        "sync" => Fault::CrashSync,
        "sql" => Fault::CrashSql,
        "commit" => Fault::CrashSegmentCommit,
        "unlink" => Fault::CrashSegmentUnlink,
        _ => Fault::CrashSegmentDelete,
    });
    store
        .write(version.id, &[(0, data(&version.meta, 0))])
        .unwrap();
    store.remove_segment(version.id, 0).unwrap();
    panic!("crash point not reached");
}
#[test]
fn segment_crash_recovery_never_revives_uncommitted_or_evicted_blocks() {
    for point in ["sync", "sql", "commit", "delete", "unlink"] {
        let dir = Directory::new();
        let result = std::process::Command::new(std::env::current_exe().unwrap())
            .args([
                "resource_cache::store::segments::tests::segment_crash_worker",
                "--exact",
                "--ignored",
            ])
            .env("QCM_SEGMENT_CRASH_ROOT", &dir.0)
            .env("QCM_SEGMENT_CRASH_POINT", point)
            .output()
            .unwrap();
        assert_eq!(
            result.status.code(),
            Some(42),
            "{}",
            String::from_utf8_lossy(&result.stderr)
        );
        let mut store = Storage::open(dir.0.clone()).unwrap();
        let version = store.lookup("segments").unwrap().unwrap();
        if point == "commit" {
            assert_eq!(
                store.read(version.id, 0).unwrap(),
                Some(data(&version.meta, 0))
            );
        } else {
            assert!(store.read(version.id, 0).unwrap().is_none());
            assert_eq!(store.usage().unwrap().audio, 0);
        }
    }
}

#[test]
#[ignore = "local segment size comparison"]
fn compare_segment_sizes() {
    for size in [16 * 1024 * 1024, 32 * 1024 * 1024] {
        for run in 0..3 {
            let dir = Directory::new();
            let mut store = Storage::open(dir.0.clone()).unwrap();
            let mut meta = metadata(116919795);
            meta.layout = Layout::Segments { bytes: size };
            let version = store.create_layout(meta.clone()).unwrap();
            let started = std::time::Instant::now();
            for start in (0..64 * 1024 * 1024 / meta.geometry.block).step_by(4) {
                store
                    .write(
                        version.id,
                        &(start..start + 4)
                            .map(|i| (i, data(&meta, i)))
                            .collect::<Vec<_>>(),
                    )
                    .unwrap();
            }
            let write_us = started.elapsed().as_micros();
            let started = std::time::Instant::now();
            for i in 0..64 * 1024 * 1024 / meta.geometry.block {
                assert_eq!(store.read(version.id, i).unwrap(), Some(data(&meta, i)));
            }
            let read_us = started.elapsed().as_micros();
            let peak = store.usage().unwrap();
            let files = fs::read_dir(&dir.0)
                .unwrap()
                .filter(|e| {
                    e.as_ref()
                        .unwrap()
                        .path()
                        .extension()
                        .is_some_and(|x| x == "seg")
                })
                .count();
            let _window = store.pin_segments(
                version.id,
                (64 * 1024 * 1024 - 1) / size..(64 * 1024 * 1024 - 1) / size + 1,
            );
            store.capacity = peak.audio + peak.metadata + 64 * 1024;
            store.evict(4 * 1024 * 1024).unwrap();
            let after = store.usage().unwrap();
            eprintln!(
                "segment_size_metrics={}",
                serde_json::json!({"size":size,"run":run,"write_us":write_us,"read_us":read_us,"files":files,"disk_audio":peak.audio,"evicted_audio":peak.audio-after.audio})
            );
        }
    }
}

#[tokio::test]
async fn legacy_long_blocks_do_not_start_automatic_materialization() {
    let dir = Directory::new();
    let store = Store::open(dir.0.clone()).await.unwrap();
    let meta = metadata(format::SEGMENT_BYTES + 17);
    let version = store.call(move |s| s.create_layout(meta)).await.unwrap();
    let blocks = (0..version.meta.geometry.count())
        .map(|i| (i, data(&version.meta, i)))
        .collect();
    assert!(store.write(version.clone(), blocks).await.unwrap().complete);
    tokio::time::sleep(std::time::Duration::from_millis(1200)).await;
    assert!(!store.plain_ready(version.clone()).await.unwrap());
    assert!(dir
        .0
        .join(format!("{}.data", version.meta.generation))
        .is_file());
    assert!(!dir
        .0
        .join(format!("{}.ok.tmp", version.meta.generation))
        .exists());
    assert!(!dir
        .0
        .join(format!("{}.ok", version.meta.generation))
        .exists());
    store.shutdown().await.unwrap();
}
