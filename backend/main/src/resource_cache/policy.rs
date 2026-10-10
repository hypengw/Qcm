use reqwest::header::{HeaderMap, HeaderName, HeaderValue};
use serde::{Deserialize, Serialize};
use std::time::{Duration, SystemTime, UNIX_EPOCH};

pub type Headers = Vec<(String, String)>;
pub fn now() -> u64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs()
}
pub fn value<'a>(headers: &'a HeaderMap, key: &str) -> Option<&'a str> {
    headers.get(key)?.to_str().ok()
}
pub fn restore(headers: &Headers) -> HeaderMap {
    let mut result = HeaderMap::new();
    for (key, val) in headers {
        if let (Ok(key), Ok(val)) = (
            HeaderName::from_bytes(key.as_bytes()),
            HeaderValue::from_str(val),
        ) {
            result.append(key, val);
        }
    }
    result
}
pub fn saved(headers: &HeaderMap) -> Headers {
    forwarded(headers)
        .iter()
        .filter(|(k, _)| !matches!(k.as_str(), "content-length" | "content-range"))
        .filter_map(|(k, v)| Some((k.to_string(), v.to_str().ok()?.to_string())))
        .collect()
}
pub fn forwarded(headers: &HeaderMap) -> HeaderMap {
    let connection = value(headers, "connection")
        .unwrap_or("")
        .split(',')
        .map(|s| s.trim().to_ascii_lowercase())
        .collect::<Vec<_>>();
    let mut result = headers.clone();
    for name in [
        "connection",
        "proxy-connection",
        "keep-alive",
        "proxy-authenticate",
        "proxy-authorization",
        "te",
        "trailer",
        "transfer-encoding",
        "upgrade",
        "set-cookie",
    ] {
        result.remove(name);
    }
    for name in connection {
        result.remove(name);
    }
    result
}

fn directives(headers: &HeaderMap) -> Vec<(String, Option<String>)> {
    let mut result = Vec::new();
    for header in headers.get_all("cache-control") {
        let Ok(text) = header.to_str() else {
            result.push(("no-store".into(), None));
            continue;
        };
        let mut quoted = false;
        let mut start = 0;
        for (i, c) in text
            .char_indices()
            .chain(std::iter::once((text.len(), ',')))
        {
            if c == '"' {
                quoted = !quoted;
            }
            if c == ',' && !quoted {
                let part = text[start..i].trim();
                start = i + 1;
                let (name, arg) = part
                    .split_once('=')
                    .map(|(n, v)| (n, Some(v.trim().trim_matches('"').to_string())))
                    .unwrap_or((part, None));
                if !name.is_empty() {
                    result.push((name.trim().to_ascii_lowercase(), arg));
                }
            }
        }
        if quoted {
            result.push(("no-store".into(), None));
        }
    }
    result
}
fn date(headers: &HeaderMap, name: &str) -> Option<u64> {
    httpdate::parse_http_date(value(headers, name)?)
        .ok()?
        .duration_since(UNIX_EPOCH)
        .ok()
        .map(|d| d.as_secs())
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct Policy {
    pub stored: u64,
    pub initial_age: u64,
    pub lifetime: u64,
    pub no_cache: bool,
    pub store: bool,
    pub etag: Option<String>,
    pub modified: Option<String>,
}
impl Policy {
    pub fn from_response(headers: &HeaderMap, sent: u64, received: u64) -> Self {
        let ds = directives(headers);
        let has = |key: &str| ds.iter().any(|(k, _)| k == key);
        let ages: Vec<_> = ds.iter().filter(|(k, _)| k == "max-age").collect();
        let max_age = if ages.len() == 1 {
            ages[0].1.as_deref().and_then(|v| v.parse::<u64>().ok())
        } else {
            None
        };
        let lifetime = if !ages.is_empty() {
            max_age.unwrap_or(0)
        } else {
            date(headers, "expires")
                .map(|expires| expires.saturating_sub(date(headers, "date").unwrap_or(received)))
                .unwrap_or(0)
        };
        Self {
            stored: received,
            initial_age: received
                .saturating_sub(date(headers, "date").unwrap_or(received))
                .max(
                    value(headers, "age")
                        .and_then(|v| v.parse::<u64>().ok())
                        .unwrap_or(0)
                        .saturating_add(received.saturating_sub(sent)),
                ),
            lifetime,
            no_cache: has("no-cache"),
            store: headers.values().all(|v| v.to_str().is_ok())
                && !has("no-store")
                && !has("must-understand")
                && !headers
                    .get_all("vary")
                    .iter()
                    .filter_map(|v| v.to_str().ok())
                    .any(|v| v.split(',').any(|v| v.trim() == "*"))
                && value(headers, "content-encoding")
                    .is_none_or(|v| v.eq_ignore_ascii_case("identity")),
            etag: value(headers, "etag").map(str::to_string),
            modified: value(headers, "last-modified").map(str::to_string),
        }
    }
    pub fn age(&self) -> u64 {
        if now() < self.stored {
            return u64::MAX;
        }
        self.initial_age
            .saturating_add(now().saturating_sub(self.stored))
    }
    pub fn fresh(&self, request: &HeaderMap) -> bool {
        if !request.contains_key("cache-control")
            && value(request, "pragma").is_some_and(|v| v.eq_ignore_ascii_case("no-cache"))
        {
            return false;
        }
        let ds = directives(request);
        if self.no_cache || ds.iter().any(|(k, _)| k == "no-cache" || k == "no-store") {
            return false;
        }
        let mut lifetime = self.lifetime;
        for (key, arg) in ds {
            if key == "max-age" {
                lifetime = lifetime.min(arg.as_deref().and_then(|s| s.parse().ok()).unwrap_or(0));
            }
            if key == "min-fresh"
                && arg
                    .as_deref()
                    .and_then(|s| s.parse::<u64>().ok())
                    .unwrap_or(u64::MAX)
                    .saturating_add(self.age())
                    >= lifetime
            {
                return false;
            }
        }
        self.age() < lifetime
    }
    pub fn strong(&self) -> Option<&str> {
        self.etag.as_deref().filter(|s| {
            s.len() >= 2
                && s.starts_with('"')
                && s.ends_with('"')
                && !s[1..s.len() - 1].contains('"')
        })
    }
    pub fn conditional(&self) -> HeaderMap {
        let mut headers = HeaderMap::new();
        if let Some(value) = &self.etag {
            if let Ok(v) = value.parse() {
                headers.insert("if-none-match", v);
            }
        } else if let Some(value) = &self.modified {
            if let Ok(v) = value.parse() {
                headers.insert("if-modified-since", v);
            }
        }
        headers
    }
}
pub fn request_no_store(headers: &HeaderMap) -> bool {
    directives(headers).iter().any(|(k, _)| k == "no-store")
}
pub fn only_cached(headers: &HeaderMap) -> bool {
    directives(headers)
        .iter()
        .any(|(k, _)| k == "only-if-cached")
}

pub fn merge_validation(old: &Headers, new: &HeaderMap) -> HeaderMap {
    let mut merged = restore(old);
    for (name, _) in saved(new) {
        merged.remove(&name);
        for val in new.get_all(&name) {
            merged.append(
                HeaderName::from_bytes(name.as_bytes()).unwrap(),
                val.clone(),
            );
        }
    }
    merged.remove("age");
    if !new.contains_key("date") {
        merged.insert(
            "date",
            httpdate::fmt_http_date(SystemTime::now()).parse().unwrap(),
        );
    }
    if let Some(age) = new.get("age") {
        merged.insert("age", age.clone());
    }
    merged
}

pub fn not_modified(policy: &Policy, request: &HeaderMap) -> bool {
    if let Some(tags) = value(request, "if-none-match") {
        return tags.split(',').any(|tag| {
            tag.trim() == "*"
                || policy.etag.as_deref().is_some_and(|etag| {
                    tag.trim().trim_start_matches("W/") == etag.trim_start_matches("W/")
                })
        });
    }
    if let (Some(since), Some(modified)) = (
        date(request, "if-modified-since"),
        policy
            .modified
            .as_deref()
            .and_then(|s| httpdate::parse_http_date(s).ok()),
    ) {
        return modified <= UNIX_EPOCH + Duration::from_secs(since);
    }
    false
}
