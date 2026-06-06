use std::{collections::BTreeMap, path::PathBuf};

#[derive(Debug, Clone)]
pub struct Piece {
    pub offset: u64,
    pub length: u64,
}

pub struct FileMeta {
    pub path: PathBuf,
    pub len: u64,
    pub pieces: BTreeMap<u64, Piece>,
}

impl FileMeta {
    pub fn longest_piece(&self, start: u64) -> Option<Piece> {
        self.pieces
            .range(..=start)
            .next_back()
            .filter(|(_, p)| p.offset + p.length > start)
            .map(|(_, p)| p.clone())
    }

    pub fn piece_of(&self, start: u64) -> Option<Piece> {
        self.longest_piece(start).map(|mut p| {
            p.length = p.offset + p.length - start;
            p.offset = start;
            p
        })
    }

    pub fn combine(&mut self, p: Piece) -> bool {
        let mut merged = p.clone();
        let mut to_remove = Vec::new();

        for (offset, existing) in self.pieces.range(..) {
            if existing.offset <= p.offset
                && existing.offset + existing.length >= p.offset + p.length
            {
                return false;
            }

            if (existing.offset <= p.offset && existing.offset + existing.length >= p.offset)
                || (p.offset <= existing.offset && p.offset + p.length >= existing.offset)
            {
                let old_offset = merged.offset;
                merged.offset = merged.offset.min(existing.offset);
                merged.length = (old_offset + merged.length).max(existing.offset + existing.length)
                    - merged.offset;
                to_remove.push(*offset);
            }
        }

        for offset in to_remove {
            self.pieces.remove(&offset);
        }
        self.pieces.insert(merged.offset, merged);
        return true;
    }

    pub fn is_end(&self) -> bool {
        return self.pieces.len() == 1
            && self
                .pieces
                .get(&0)
                .map(|v| v.offset == 0 && v.length == self.len)
                .unwrap_or(false);
    }
}
