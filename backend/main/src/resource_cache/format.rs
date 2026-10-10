use anyhow::{bail, ensure, Result};
use bytes::Bytes;
use qcm_core::crypto::{digest, MessageDigest};
use std::ops::Range;

pub const MIN_BLOCK: u64 = 256 * 1024;
pub const MAX_BLOCK: u64 = 1024 * 1024;
pub const HEADER_LEN: u64 = 64;
pub const ENTRY_LEN: usize = 36;
const MAGIC: &[u8; 8] = b"QCMRSC01";

pub fn hash(data: &[u8]) -> Result<[u8; 32]> {
    digest(MessageDigest::sha256(), data)
        .map_err(|e| anyhow::anyhow!("{e}"))?
        .try_into()
        .map_err(|_| anyhow::anyhow!("invalid SHA-256 length"))
}

pub fn identity(data: &[u8]) -> Result<String> {
    Ok(hash(data)?.iter().map(|b| format!("{b:02x}")).collect())
}

#[derive(Clone, Copy, Debug, serde::Serialize, serde::Deserialize)]
pub struct Geometry {
    pub length: u64,
    pub block: u64,
}

impl Geometry {
    pub fn new(length: u64) -> Result<Self> {
        let mut block = MIN_BLOCK;
        while length.div_ceil(block) > 8192 && block < MAX_BLOCK {
            block *= 2;
        }
        let value = Self { length, block };
        ensure!(
            value.count() <= 1_000_000,
            "resource index exceeds memory limit"
        );
        Ok(value)
    }
    pub fn count(self) -> u64 {
        self.length.div_ceil(self.block)
    }
    pub fn range(self, index: u64) -> Result<Range<u64>> {
        ensure!(index < self.count(), "block out of bounds");
        Ok(index * self.block..((index + 1) * self.block).min(self.length))
    }
    pub fn offset(self, slot: u32) -> Result<u64> {
        HEADER_LEN
            .checked_add(
                u64::from(slot)
                    .checked_mul(self.block)
                    .ok_or_else(|| anyhow::anyhow!("slot overflow"))?,
            )
            .ok_or_else(|| anyhow::anyhow!("offset overflow"))
    }
    pub fn header(self, generation: &str) -> Result<[u8; 64]> {
        let mut bytes = [0; 64];
        bytes[..8].copy_from_slice(MAGIC);
        bytes[8..16].copy_from_slice(&self.length.to_le_bytes());
        bytes[16..24].copy_from_slice(&self.block.to_le_bytes());
        bytes[24..56].copy_from_slice(&hash(generation.as_bytes())?);
        Ok(bytes)
    }
}

pub struct Index {
    bytes: Vec<u8>,
    dirty: Vec<Range<usize>>,
    geometry: Geometry,
}

impl Index {
    pub fn new(geometry: Geometry) -> Self {
        Self {
            bytes: vec![0; geometry.count() as usize * ENTRY_LEN],
            dirty: Vec::new(),
            geometry,
        }
    }
    pub fn load(geometry: Geometry, bytes: Vec<u8>, slots: u64) -> Result<Self> {
        ensure!(
            matches!(geometry.block, MIN_BLOCK | 524288 | MAX_BLOCK),
            "invalid block size"
        );
        ensure!(
            geometry.count() <= 1_000_000 && bytes.len() == geometry.count() as usize * ENTRY_LEN,
            "invalid index size"
        );
        let value = Self {
            bytes,
            dirty: Vec::new(),
            geometry,
        };
        for i in 0..geometry.count() {
            if let Some((slot, _)) = value.get(i) {
                ensure!(u64::from(slot) < slots, "index references absent slot");
            }
        }
        Ok(value)
    }
    pub fn get(&self, index: u64) -> Option<(u32, [u8; 32])> {
        let start = usize::try_from(index).ok()?.checked_mul(ENTRY_LEN)?;
        let entry = self.bytes.get(start..start.checked_add(ENTRY_LEN)?)?;
        let slot = u32::from_le_bytes(entry[..4].try_into().ok()?);
        (slot != 0).then(|| (slot - 1, entry[4..].try_into().unwrap()))
    }
    pub fn set(&mut self, index: u64, slot: u32, data: &Bytes) -> Result<()> {
        let range = self.geometry.range(index)?;
        ensure!(
            data.len() as u64 == range.end - range.start,
            "incomplete block"
        );
        if slot == u32::MAX {
            bail!("physical slot limit reached");
        }
        let start = index as usize * ENTRY_LEN;
        self.bytes[start..start + 4].copy_from_slice(&(slot + 1).to_le_bytes());
        self.bytes[start + 4..start + ENTRY_LEN].copy_from_slice(&hash(data)?);
        self.dirty.push(start..start + ENTRY_LEN);
        Ok(())
    }
    pub fn remove(&mut self, index: u64) {
        let start = index as usize * ENTRY_LEN;
        self.bytes[start..start + ENTRY_LEN].fill(0);
        self.dirty.push(start..start + ENTRY_LEN);
    }
    pub fn complete(&self) -> bool {
        (0..self.geometry.count()).all(|i| self.get(i).is_some())
    }
    pub fn dirty(&mut self) -> Vec<Range<usize>> {
        self.dirty.sort_by_key(|r| r.start);
        let mut ranges: Vec<Range<usize>> = Vec::new();
        for range in &self.dirty {
            if let Some(last) = ranges.last_mut() {
                if range.start <= last.end {
                    last.end = last.end.max(range.end);
                    continue;
                }
            }
            ranges.push(range.clone());
        }
        ranges
    }
    pub fn bytes(&self) -> &[u8] {
        &self.bytes
    }
    pub fn committed(&mut self) {
        self.dirty.clear();
    }
}
