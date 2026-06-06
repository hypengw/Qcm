use super::error::HttpError;
use http_range_header::parse_range_header;
use std::fmt;

#[derive(Clone, Debug, PartialEq)]
pub struct HttpRange {
    inner: http_range_header::SyntacticallyCorrectRange,
}

impl Default for HttpRange {
    fn default() -> Self {
        Self {
            inner: http_range_header::SyntacticallyCorrectRange {
                start: http_range_header::StartPosition::Index(0),
                end: http_range_header::EndPosition::LastByte,
            },
        }
    }
}

impl HttpRange {
    fn from_inner(r: http_range_header::SyntacticallyCorrectRange) -> Self {
        Self { inner: r }
    }

    pub fn start(&self, full: u64) -> u64 {
        match self.inner.start {
            http_range_header::StartPosition::Index(pos) => pos,
            http_range_header::StartPosition::FromLast(pos) => full - pos,
        }
    }

    pub fn in_full(&self, full: u64) -> bool {
        match self.inner.start {
            http_range_header::StartPosition::Index(offset) => {
                if full <= offset {
                    return false;
                }
            }
            http_range_header::StartPosition::FromLast(offset) => {
                if full <= offset {
                    return false;
                }
            }
        }
        match self.inner.end {
            http_range_header::EndPosition::Index(offset) => {
                if full <= offset {
                    return false;
                }
            }
            http_range_header::EndPosition::LastByte => {
                if full == 0 {
                    return false;
                }
            }
        }
        return true;
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct HttpContentRange {
    pub start: u64,
    pub end: u64,
    pub full: u64,
}

impl HttpContentRange {
    /// not check range if in full
    pub fn from_range(r: HttpRange, full: u64) -> Option<HttpContentRange> {
        if full == 0 {
            None
        } else {
            Some(HttpContentRange {
                start: match r.inner.start {
                    http_range_header::StartPosition::Index(offset) => offset,
                    http_range_header::StartPosition::FromLast(offset) => full - offset,
                },
                end: match r.inner.end {
                    http_range_header::EndPosition::Index(offset) => offset,
                    http_range_header::EndPosition::LastByte => full - 1,
                },
                full,
            })
        }
    }
}

impl fmt::Display for HttpContentRange {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "bytes {}-{}/{}", self.start, self.end, self.full)
    }
}

impl fmt::Display for HttpRange {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match (self.inner.start, self.inner.end) {
            (
                http_range_header::StartPosition::Index(start),
                http_range_header::EndPosition::Index(end),
            ) => {
                write!(f, "bytes={}-{}", start, end)
            }
            (
                http_range_header::StartPosition::Index(start),
                http_range_header::EndPosition::LastByte,
            ) => {
                write!(f, "bytes={}-", start)
            }
            (http_range_header::StartPosition::FromLast(start), _) => {
                write!(f, "bytes=-{}", start)
            }
        }
    }
}

pub fn parse_content_range(s: &str) -> Option<HttpContentRange> {
    let parts: Vec<&str> = s.split(['/', ' ']).collect();
    match parts.as_slice() {
        ["bytes", range, full] => match range.split("-").collect::<Vec<&str>>().as_slice() {
            [start, end] => {
                if let (Ok(start), Ok(end), Ok(full)) = (start.parse(), end.parse(), full.parse()) {
                    Some(HttpContentRange { start, end, full })
                } else {
                    None
                }
            }
            _ => None,
        },
        _ => None,
    }
}

pub fn parse_range(s: &str) -> Result<HttpRange, HttpError> {
    match parse_range_header(s) {
        Ok(parsed) => {
            if parsed.ranges.len() == 1 {
                let r = parsed.ranges[0];
                Ok(HttpRange::from_inner(r))
            } else {
                Err(HttpError::UnsupportedRange(s.to_string()))
            }
        }
        Err(_) => Err(HttpError::UnsupportedRange(s.to_string())),
    }
}
