use super::*;
use std::{collections::BTreeMap, ops::Range};

// These guards protect only a bounded working window, independently of Version identity.
pub struct Window {
    _pins: Vec<Arc<()>>,
}
#[derive(Clone)]
struct Segment {
    number: u64,
    token: String,
    length: u64,
    deleting: bool,
}
impl Segment {
    fn name(&self, meta: &Metadata) -> String {
        format!(
            "{}.{:016x}.{}.seg",
            meta.generation, self.number, self.token
        )
    }
    fn header(&self, meta: &Metadata) -> Result<[u8; 64]> {
        let mut header = [0; 64];
        header[..8].copy_from_slice(b"QCMSEG01");
        header[8..16].copy_from_slice(&self.length.to_le_bytes());
        header[16..24].copy_from_slice(&self.number.to_le_bytes());
        header[24..32].copy_from_slice(&meta.geometry.block.to_le_bytes());
        header[32..48].copy_from_slice(uuid::Uuid::parse_str(&self.token)?.as_bytes());
        header[48..].copy_from_slice(&format::hash(meta.generation.as_bytes())?[..16]);
        Ok(header)
    }
}
pub(super) fn managed_name(name: &str) -> bool {
    let parts: Vec<_> = name.split('.').collect();
    parts.len() == 4
        && valid_generation(parts[0])
        && parts[1].len() == 16
        && parts[1].bytes().all(|b| b.is_ascii_hexdigit())
        && valid_generation(parts[2])
        && parts[3] == "seg"
}
impl Store {
    pub async fn window(&self, version: Version, range: Range<u64>) -> Result<Window> {
        self.call_version(version, move |s, version| {
            s.check_active(version.id)?;
            let meta = s.load(version.id)?.meta.clone();
            ensure!(
                range.start <= range.end
                    && range.end <= meta.geometry.length
                    && range.end - range.start <= format::WINDOW_BYTES,
                "invalid cache window"
            );
            let Some(size) = meta.layout.segment_bytes() else {
                return Ok(Window { _pins: vec![] });
            };
            Ok(s.pin_segments(version.id, range.start / size..range.end.div_ceil(size)))
        })
        .await
    }
}
impl Storage {
    pub(super) fn restore_segment_clock(&mut self) -> Result<()> {
        let mut row = self
            .db
            .statement("SELECT COALESCE(MAX(last_use),0) FROM segments", &[])?;
        row.next()?;
        self.segment_clock = row.int(0).max(0) as u64;
        Ok(())
    }
    fn segment_tick(&mut self) -> u64 {
        let now = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap_or_default()
            .as_micros()
            .min(i64::MAX as u128) as u64;
        self.segment_clock = now.max(self.segment_clock.saturating_add(1));
        self.segment_clock
    }
    fn pin_segments(&mut self, id: i64, numbers: Range<u64>) -> Window {
        self.pins.retain(|_, pin| pin.strong_count() > 0);
        let pins = numbers
            .map(|number| {
                let pin = self
                    .pins
                    .get(&(id, number))
                    .and_then(Weak::upgrade)
                    .unwrap_or_else(|| Arc::new(()));
                self.pins.insert((id, number), Arc::downgrade(&pin));
                pin
            })
            .collect();
        Window { _pins: pins }
    }
    fn segment(&self, id: i64, number: u64) -> Result<Option<Segment>> {
        let mut row = self.db.statement(
            "SELECT token,length,deleting FROM segments WHERE version_id=? AND number=?",
            &[Value::Int(id), Value::Int(i64::try_from(number)?)],
        )?;
        if !row.next()? {
            return Ok(None);
        }
        let token = row.text(0)?;
        ensure!(
            valid_generation(&token) && row.int(1) > 0,
            "invalid segment descriptor"
        );
        Ok(Some(Segment {
            number,
            token,
            length: row.int(1) as u64,
            deleting: row.int(2) != 0,
        }))
    }
    fn segment_file(&self, meta: &Metadata, segment: &Segment) -> Result<File> {
        let size = meta
            .layout
            .segment_bytes()
            .ok_or_else(|| anyhow::anyhow!("not segmented"))?;
        let start = segment
            .number
            .checked_mul(size)
            .ok_or_else(|| anyhow::anyhow!("segment overflow"))?;
        ensure!(
            start < meta.geometry.length
                && segment.length == size.min(meta.geometry.length - start)
                && !segment.deleting,
            "invalid segment length"
        );
        let path = self.root.join(segment.name(meta));
        materialize::regular_file(&path)?;
        let mut file = OpenOptions::new().read(true).write(true).open(path)?;
        let length = file.metadata()?.len();
        ensure!(
            length >= HEADER_LEN && length <= HEADER_LEN + segment.length,
            "invalid segment file length"
        );
        let mut header = [0; HEADER_LEN as usize];
        file.read_exact(&mut header)?;
        ensure!(header == segment.header(meta)?, "segment identity mismatch");
        Ok(file)
    }
    pub(super) fn retain_segments(&self, retained: &mut HashMap<String, i32>) -> Result<()> {
        let mut rows = self.db.statement("SELECT v.meta,s.number,s.token,s.length FROM segments s JOIN versions v ON v.id=s.version_id", &[])?;
        while rows.next()? {
            let Ok(meta) = serde_json::from_str::<Metadata>(&rows.text(0)?) else {
                continue;
            };
            if !valid_generation(&meta.generation) || meta.layout.segment_bytes().is_none() {
                continue;
            }
            let token = rows.text(2)?;
            if rows.int(1) < 0 || !valid_generation(&token) {
                continue;
            }
            let segment = Segment {
                number: rows.int(1) as u64,
                token,
                length: rows.int(3) as u64,
                deleting: false,
            };
            retained.insert(segment.name(&meta), meta.kind);
        }
        Ok(())
    }
    pub(super) fn repair_segments(&mut self, id: i64) -> Result<()> {
        let meta = self.load(id)?.meta.clone();
        let Some(size) = meta.layout.segment_bytes() else {
            return Ok(());
        };
        let count = meta.geometry.length.div_ceil(size);
        for number in 0..count {
            let first = number * size / meta.geometry.block;
            let end = ((number + 1) * size)
                .min(meta.geometry.length)
                .div_ceil(meta.geometry.block);
            if !(first..end).any(|i| self.versions[&id].index.get(i).is_some()) {
                continue;
            }
            let valid = self
                .segment(id, number)
                .ok()
                .flatten()
                .is_some_and(|s| self.segment_file(&meta, &s).is_ok());
            if !valid {
                self.remove_segment(id, number)?;
            }
        }
        Ok(())
    }
    fn publish_segment_index(
        &mut self,
        id: i64,
        mut index: Index,
        operation: impl FnOnce(&Database) -> Result<()>,
    ) -> Result<()> {
        let ranges = index.dirty();
        self.db.exec("BEGIN IMMEDIATE")?;
        let result = (|| {
            self.db.patch(id, index.bytes(), &ranges)?;
            operation(&self.db)?;
            #[cfg(test)]
            {
                self.crash_at(Fault::CrashSql)?;
                self.fail_at(Fault::Sql)?;
            }
            self.db.exec("COMMIT")
        })();
        if let Err(err) = result {
            let _ = self.db.exec("ROLLBACK");
            return Err(err);
        }
        #[cfg(test)]
        self.crash_at(Fault::CrashSegmentCommit)?;
        index.committed();
        self.versions.get_mut(&id).unwrap().index = index;
        Ok(())
    }
    pub(super) fn read_segment(&mut self, id: i64, block: u64) -> Result<Option<Bytes>> {
        let value = self.load(id)?;
        let meta = value.meta.clone();
        let Some((slot, digest)) = value.index.get(block) else {
            return Ok(None);
        };
        let range = meta.geometry.range(block)?;
        let number = range.start / meta.layout.segment_bytes().unwrap();
        let Some(segment) = self.segment(id, number)? else {
            self.remove_segment(id, number)?;
            return Ok(None);
        };
        let mut file = match self.segment_file(&meta, &segment) {
            Ok(file) => file,
            Err(_) => {
                self.remove_segment(id, number)?;
                return Ok(None);
            }
        };
        let mut bytes = vec![0; (range.end - range.start) as usize];
        file.seek(SeekFrom::Start(
            HEADER_LEN + u64::from(slot) * meta.geometry.block,
        ))?;
        if file.read_exact(&mut bytes).is_err() || format::hash(&bytes)? != digest {
            let mut index = self.versions[&id].index.clone();
            index.remove(block);
            self.publish_segment_index(id, index, |_| Ok(()))?;
            return Ok(None);
        }
        let touched = self.segment_tick();
        self.db
            .statement(
                "UPDATE segments SET last_use=? WHERE version_id=? AND number=?",
                &[
                    Value::Int(touched as i64),
                    Value::Int(id),
                    Value::Int(number as i64),
                ],
            )?
            .next()?;
        Ok(Some(Bytes::from(bytes)))
    }
    pub(super) fn write_segments(&mut self, id: i64, blocks: &[(u64, Bytes)]) -> Result<()> {
        self.repair_segments(id)?;
        let meta = self.versions[&id].meta.clone();
        let size = meta.layout.segment_bytes().unwrap();
        let mut grouped: BTreeMap<u64, BTreeMap<u64, &Bytes>> = BTreeMap::new();
        for (block, data) in blocks {
            let range = meta.geometry.range(*block)?;
            ensure!(
                data.len() as u64 == range.end - range.start,
                "incomplete block"
            );
            if self.versions[&id].index.get(*block).is_none() {
                grouped
                    .entry(range.start / size)
                    .or_default()
                    .insert(*block, data);
            }
        }
        if grouped.is_empty() {
            return Ok(());
        }
        let _pins: Vec<_> = grouped
            .keys()
            .map(|n| self.pin_segments(id, *n..*n + 1))
            .collect();
        let required: u64 = grouped
            .values()
            .map(|v| {
                v.values()
                    .map(|b| (b.len() as u64).div_ceil(4096) * 4096)
                    .sum::<u64>()
                    + 8192
            })
            .sum();
        self.evict(required)?;
        #[cfg(test)]
        self.fail_at(Fault::Write)?;
        let mut index = self.versions[&id].index.clone();
        let mut written = Vec::new();
        let mut created = Vec::new();
        let result = (|| {
            for (number, blocks) in grouped {
                let segment = match self.segment(id, number)? {
                    Some(segment) if !segment.deleting => segment,
                    old => {
                        if old.is_some() {
                            self.remove_segment(id, number)?;
                        }
                        let segment = Segment {
                            number,
                            token: uuid::Uuid::new_v4().simple().to_string(),
                            length: size.min(meta.geometry.length - number * size),
                            deleting: false,
                        };
                        created.push(segment.name(&meta));
                        segment
                    }
                };
                let name = segment.name(&meta);
                let mut file = if created.last() == Some(&name) {
                    let mut file = OpenOptions::new()
                        .create_new(true)
                        .read(true)
                        .write(true)
                        .open(self.root.join(&name))?;
                    materialize::private_file(&file)?;
                    file.write_all(&segment.header(&meta)?)?;
                    file
                } else {
                    self.segment_file(&meta, &segment)?
                };
                let writing = (|| {
                    for (block, data) in blocks {
                        let slot = (block % (size / meta.geometry.block)) as u32;
                        file.seek(SeekFrom::Start(
                            HEADER_LEN + u64::from(slot) * meta.geometry.block,
                        ))?;
                        file.write_all(data)?;
                        index.set(block, slot, data)?;
                    }
                    #[cfg(test)]
                    self.fail_at(Fault::Sync)?;
                    file.sync_all()?;
                    #[cfg(test)]
                    self.crash_at(Fault::CrashSync)?;
                    Ok::<_, anyhow::Error>(())
                })();
                self.sizes
                    .insert(name, (allocated(&file.metadata()?), meta.kind));
                writing?;
                written.push(segment);
            }
            sync_directory(&self.root)?;
            let touched = self.segment_tick();
            self.publish_segment_index(id, index, |db| {
                for segment in written {
                    db.statement("INSERT INTO segments(version_id,number,token,length,last_use,deleting) VALUES(?,?,?,?,?,0) ON CONFLICT(version_id,number) DO UPDATE SET token=excluded.token,length=excluded.length,last_use=excluded.last_use,deleting=0",
                        &[Value::Int(id), Value::Int(segment.number as i64), Value::Text(&segment.token), Value::Int(segment.length as i64), Value::Int(touched as i64)])?.next()?;
                }
                Ok(())
            })
        })();
        if result.is_err() {
            for name in created {
                let _ = self.remove_managed(&name);
            }
        }
        result
    }
    fn remove_segment(&mut self, id: i64, number: u64) -> Result<()> {
        let meta = self.load(id)?.meta.clone();
        let size = meta
            .layout
            .segment_bytes()
            .ok_or_else(|| anyhow::anyhow!("not segmented"))?;
        let mut index = self.versions[&id].index.clone();
        for block in number * size / meta.geometry.block
            ..((number + 1) * size)
                .min(meta.geometry.length)
                .div_ceil(meta.geometry.block)
        {
            if index.get(block).is_some() {
                index.remove(block);
            }
        }
        self.publish_segment_index(id, index, |db| {
            db.statement(
                "UPDATE segments SET deleting=1 WHERE version_id=? AND number=?",
                &[Value::Int(id), Value::Int(number as i64)],
            )?
            .next()?;
            Ok(())
        })?;
        #[cfg(test)]
        {
            self.crash_at(Fault::CrashSegmentDelete)?;
            self.fail_at(Fault::SegmentDelete)?;
        }
        if let Ok(Some(segment)) = self.segment(id, number) {
            self.remove_managed(&segment.name(&meta))?;
            #[cfg(test)]
            self.crash_at(Fault::CrashSegmentUnlink)?;
        }
        self.db
            .statement(
                "DELETE FROM segments WHERE version_id=? AND number=?",
                &[Value::Int(id), Value::Int(number as i64)],
            )?
            .next()?;
        Ok(())
    }
    pub(super) fn delete_all_segments(&mut self, id: i64) -> Result<()> {
        let mut rows = self.db.statement(
            "SELECT number FROM segments WHERE version_id=?",
            &[Value::Int(id)],
        )?;
        let mut numbers = Vec::new();
        while rows.next()? {
            numbers.push(rows.int(0) as u64);
        }
        drop(rows);
        for number in numbers {
            self.remove_segment(id, number)?;
        }
        Ok(())
    }
    pub(super) fn evict_segments(&mut self, required: u64) -> Result<()> {
        let mut rows = self.db.statement("SELECT s.version_id,s.number,s.deleting,v.obsolete FROM segments s JOIN versions v ON v.id=s.version_id ORDER BY s.deleting DESC,v.obsolete DESC,s.last_use ASC,s.rowid ASC", &[])?;
        let mut candidates = Vec::new();
        while rows.next()? {
            candidates.push((
                rows.int(0),
                rows.int(1) as u64,
                rows.int(2) != 0 || rows.int(3) != 0,
            ));
        }
        drop(rows);
        for (id, number, obsolete) in candidates {
            let usage = self.usage()?;
            if !obsolete && usage.image + usage.audio + usage.metadata + required <= self.capacity {
                continue;
            }
            if self
                .pins
                .get(&(id, number))
                .and_then(Weak::upgrade)
                .is_some()
            {
                continue;
            }
            if let Err(err) = self.remove_segment(id, number) {
                log::warn!("segment deletion deferred: {err}");
            }
        }
        Ok(())
    }
}

#[cfg(test)]
#[path = "segment_tests.rs"]
mod tests;
