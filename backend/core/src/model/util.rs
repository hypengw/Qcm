use chrono::{DateTime, Utc};

pub fn epoch() -> DateTime<Utc> {
    return DateTime::from_timestamp_millis(0).unwrap();
}

pub fn default_true() -> bool {
    return true;
}

pub fn default_one() -> i32 { 1 }

pub fn default_json_arr() -> serde_json::Value {
    return serde_json::json!([]);
}

pub fn default_language() -> String {
    return "und".to_string();
}
