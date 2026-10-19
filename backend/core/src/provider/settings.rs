use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::collections::{BTreeMap, HashSet};

pub type SettingValues = BTreeMap<String, Value>;

#[derive(Clone, Debug, Default, Serialize, Deserialize)]
pub struct SettingsSnapshot {
    pub revision: u64,
    pub values: SettingValues,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum SettingKind {
    Boolean,
    Choice,
    Integer,
    Text,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct SettingOption {
    pub value: Value,
    pub name: String,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct SettingCondition {
    pub key: String,
    pub value: Value,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct SettingField {
    pub key: String,
    pub name: String,
    #[serde(default)]
    pub group: String,
    #[serde(default)]
    pub description: String,
    pub kind: SettingKind,
    pub default_value: Value,
    #[serde(default)]
    pub options: Vec<SettingOption>,
    pub minimum: Option<i64>,
    pub maximum: Option<i64>,
    pub enabled_when: Option<SettingCondition>,
}

#[derive(Clone, Debug, Default, thiserror::Error)]
#[error("{message}")]
pub struct SettingsError {
    pub message: String,
    pub fields: BTreeMap<String, String>,
}

impl SettingsError {
    pub fn message(message: impl Into<String>) -> Self {
        Self {
            message: message.into(),
            fields: BTreeMap::new(),
        }
    }
}

pub fn validate(
    fields: &[SettingField],
    values: &SettingValues,
) -> Result<SettingValues, SettingsError> {
    if fields.len() > 64 || serde_json::to_vec(values).map_or(true, |v| v.len() > 16 * 1024) {
        return Err(SettingsError::message("Provider settings are too large"));
    }
    let mut keys = HashSet::new();
    for field in fields {
        if field.key.is_empty() || field.key.len() > 64 || !keys.insert(field.key.as_str()) {
            return Err(SettingsError::message(
                "Invalid provider settings definition",
            ));
        }
        if let (Some(min), Some(max)) = (field.minimum, field.maximum) {
            if min > max {
                return Err(SettingsError::message("Invalid integer range"));
            }
        }
        check(field, &field.default_value).map_err(SettingsError::message)?;
    }
    for field in fields {
        if let Some(condition) = &field.enabled_when {
            if condition.key == field.key || !keys.contains(condition.key.as_str()) {
                return Err(SettingsError::message("Invalid settings condition"));
            }
            let dependency = fields
                .iter()
                .find(|field| field.key == condition.key)
                .unwrap();
            check(dependency, &condition.value).map_err(SettingsError::message)?;
        }
    }
    let mut result = SettingValues::new();
    let mut errors = BTreeMap::new();
    for key in values.keys() {
        if !keys.contains(key.as_str()) {
            errors.insert(key.clone(), "Unknown setting".into());
        }
    }
    for field in fields {
        let value = values.get(&field.key).unwrap_or(&field.default_value);
        if let Err(error) = check(field, value) {
            errors.insert(field.key.clone(), error);
        } else {
            result.insert(field.key.clone(), value.clone());
        }
    }
    if serde_json::to_vec(&result).map_or(true, |v| v.len() > 16 * 1024) {
        return Err(SettingsError::message("Provider settings are too large"));
    }
    if errors.is_empty() {
        Ok(result)
    } else {
        Err(SettingsError {
            message: "Invalid provider settings".into(),
            fields: errors,
        })
    }
}

fn check(field: &SettingField, value: &Value) -> Result<(), String> {
    let valid = match field.kind {
        SettingKind::Boolean => value.is_boolean(),
        SettingKind::Text => value.as_str().is_some_and(|v| v.len() <= 4096),
        SettingKind::Integer => value.as_i64().is_some_and(|n| {
            n.abs_diff(0) <= 9_007_199_254_740_991
                && field.minimum.is_none_or(|min| n >= min)
                && field.maximum.is_none_or(|max| n <= max)
        }),
        SettingKind::Choice => {
            (value.is_string() || value.is_boolean() || value.as_i64().is_some())
                && field.options.iter().any(|option| option.value == *value)
        }
    };
    if valid {
        Ok(())
    } else {
        Err("Value does not match the setting's type or allowed values".into())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn settings_defaults_types_ranges_and_unknown_fields() {
        let fields: Vec<SettingField> = serde_json::from_value(serde_json::json!([
            {"key":"mode","name":"Mode","kind":"choice","default_value":"hls","options":[{"value":"hls","name":"HLS"},{"value":"original","name":"Original"}]},
            {"key":"bitrate","name":"Bitrate","kind":"integer","default_value":320000,"minimum":32000,"maximum":512000,"enabled_when":{"key":"mode","value":"hls"}},
            {"key":"flag","name":"Flag","kind":"boolean","default_value":true},
            {"key":"label","name":"Label","kind":"text","default_value":""}
        ])).unwrap();
        let defaults = validate(&fields, &SettingValues::new()).unwrap();
        assert_eq!(defaults["bitrate"], 320000);
        for invalid in [
            serde_json::json!({"bitrate":0}),
            serde_json::json!({"bitrate":"320000"}),
            serde_json::json!({"mode":"auto"}),
            serde_json::json!({"flag":1}),
            serde_json::json!({"label":[]}),
            serde_json::json!({"unknown":true}),
        ] {
            assert!(
                !validate(&fields, &serde_json::from_value(invalid).unwrap())
                    .unwrap_err()
                    .fields
                    .is_empty()
            );
        }
        let mut broken = fields.clone();
        broken[1].default_value = Value::from(-1);
        assert!(validate(&broken, &SettingValues::new()).is_err());
        broken = fields.clone();
        broken[1].enabled_when.as_mut().unwrap().key = "missing".into();
        assert!(validate(&broken, &SettingValues::new()).is_err());
    }
}
