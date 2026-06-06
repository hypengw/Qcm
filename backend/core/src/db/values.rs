use chrono::{DateTime, TimeZone, Utc};
use sea_orm::{
    prelude::Expr,
    sea_query::{ArrayType, SimpleExpr, Value, ValueType, ValueTypeErr},
    ColumnType, DeriveValueType, QueryResult,
};
use serde::{Deserialize, Serialize};

#[derive(Copy, Clone, Debug, Default, PartialEq, Eq, DeriveValueType)]
pub struct Timestamp(i64);

impl Timestamp {
    pub fn new() -> Self {
        Timestamp(0)
    }
    pub fn from_millis(v: i64) -> Timestamp {
        Timestamp(v)
    }

    pub fn as_millis(&self) -> i64 {
        self.0
    }

    pub fn now() -> Timestamp {
        Timestamp(Utc::now().timestamp_millis())
    }

    pub fn now_expr() -> SimpleExpr {
        Expr::cust("(strftime('%s','now')*1000)")
    }

    pub fn default_expr() -> SimpleExpr {
        Expr::cust("(0)")
    }
}

#[derive(Clone, Debug, PartialEq, Eq, Default)]
pub struct StringVec(pub Vec<String>);

impl std::convert::From<StringVec> for Value {
    fn from(source: StringVec) -> Self {
        let jarr: serde_json::Value = source.0.into();
        jarr.into()
    }
}

impl sea_orm::TryGetable for StringVec {
    fn try_get_by<I: sea_orm::ColIdx>(
        res: &QueryResult,
        idx: I,
    ) -> Result<Self, sea_orm::TryGetError> {
        <serde_json::Value as sea_orm::TryGetable>::try_get_by(res, idx).and_then(|v| {
            match serde_json::from_value(v) {
                Ok(jarr) => {
                    let arr: Vec<String> = jarr;
                    return Ok(StringVec(arr));
                }
                Err(err) => {
                    return Err(sea_orm::TryGetError::Null(err.to_string()));
                }
            }
        })
    }
}

impl ValueType for StringVec {
    fn try_from(v: Value) -> Result<Self, ValueTypeErr> {
        <serde_json::Value as ValueType>::try_from(v).and_then(|jarr| match serde_json::from_value(
            jarr,
        ) {
            Ok(jarr) => {
                let arr: Vec<String> = jarr;
                return Ok(StringVec(arr));
            }
            Err(_) => {
                return Err(ValueTypeErr);
            }
        })
    }

    fn type_name() -> String {
        stringify!(StringVec).to_owned()
    }

    fn array_type() -> ArrayType {
        ArrayType::String
    }

    fn column_type() -> ColumnType {
        ColumnType::Json
    }
}

impl TryFrom<String> for Timestamp {
    type Error = chrono::format::ParseError;
    fn try_from(value: String) -> Result<Self, Self::Error> {
        value.parse().map(|d: DateTime<Utc>| d.into())
    }
}

impl From<DateTime<Utc>> for Timestamp {
    fn from(dt: DateTime<Utc>) -> Self {
        Timestamp(dt.timestamp_millis())
    }
}

impl From<Timestamp> for DateTime<Utc> {
    fn from(ts: Timestamp) -> Self {
        match Utc.timestamp_millis_opt(ts.0) {
            chrono::MappedLocalTime::Single(dt) => dt,
            _ => DateTime::<Utc>::from_timestamp_nanos(0),
        }
    }
}

impl Serialize for Timestamp {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        serializer.serialize_i64(self.0)
    }
}

impl<'de> Deserialize<'de> for Timestamp {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let v = i64::deserialize(deserializer)?;
        Ok(Timestamp(v))
    }
}
