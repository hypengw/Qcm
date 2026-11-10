module;
#include <rstd/macro.hpp>
export module qcm.backend:db.codec;
export import :datetime;
export import :db.timestamp;
export import liorm.sqlito;
export import rstd.json;
using namespace rstd::prelude;

export template<>
struct liorm::ValueTraits<qcm::backend::Timestamp> : liorm::ValueTraits<i64> {};

export template<>
struct liorm::sqlito::Codec<qcm::backend::Timestamp> {
    static constexpr auto parameter_type() -> liorm::query::ParameterType {
        return liorm::ValueTraits<qcm::backend::Timestamp>::type;
    }
    static auto read(::sqlito::Statement& stmt, usize column)
        -> Result<qcm::backend::Timestamp, ::sqlito::Error> {
        return Ok(qcm::backend::Timestamp::from_millis(rstd_try(Codec<i64>::read(stmt, column))));
    }
    static auto write(::sqlito::Statement& stmt, usize slot, qcm::backend::Timestamp value)
        -> Result<empty, ::sqlito::Error> {
        return Codec<i64>::write(stmt, slot, value.as_millis());
    }
};

export template<>
struct liorm::sqlito::Codec<qcm::backend::DatabaseDateTime> {
    static constexpr auto parameter_type() -> liorm::query::ParameterType {
        return Codec<String>::parameter_type();
    }
    static auto read(::sqlito::Statement& stmt, usize column)
        -> Result<qcm::backend::DatabaseDateTime, ::sqlito::Error> {
        using namespace qcm::backend;
        auto type = rstd_try(stmt.column_type(column));
        if (type == ::sqlito::ColumnType::Text) {
            auto text   = rstd_try(Codec<String>::read(stmt, column));
            auto parsed = parse_database_datetime(text.as_str());
            if (parsed.is_none()) return Err(invalid_column());
            return Ok(*parsed);
        }
        if (type == ::sqlito::ColumnType::Integer) {
            auto value = rstd_try(Codec<i64>::read(stmt, column));
            if (! datetime_valid_seconds(value.to_primitive())) return Err(invalid_column());
            return Ok(DatabaseDateTime { value, 0 });
        }
        if (type != ::sqlito::ColumnType::Real) return Err(invalid_column());
        auto value     = rstd_try(Codec<f64>::read(stmt, column));
        auto timestamp = (value - f64(2440587.5)) * f64(86400.0);
        if (! timestamp.is_finite() || timestamp < f64(-9e12) || timestamp > f64(9e12))
            return Err(invalid_column());
        auto whole   = timestamp.trunc();
        auto seconds = static_cast<long long>(whole.to_primitive());
        if (! datetime_valid_seconds(seconds)) return Err(invalid_column());
        auto nanos = (timestamp - whole).abs().to_primitive() * 1e9;
        return Ok(DatabaseDateTime { i64(seconds), static_cast<unsigned>(nanos) });
    }
    static auto write(::sqlito::Statement& stmt, usize slot,
                      const qcm::backend::DatabaseDateTime& value)
        -> Result<empty, ::sqlito::Error> {
        auto encoded = qcm::backend::encode_database_datetime(value);
        if (encoded.is_none()) return Err(invalid_column());
        return Codec<String>::write(stmt, slot, *encoded);
    }
};

export template<>
struct liorm::sqlito::Codec<rstd::json::Value> {
    static constexpr auto parameter_type() -> liorm::query::ParameterType {
        return Codec<String>::parameter_type();
    }
    static auto read(::sqlito::Statement& stmt, usize column)
        -> Result<rstd::json::Value, ::sqlito::Error> {
        auto text   = rstd_try(Codec<String>::read(stmt, column));
        auto parsed = rstd::json::from_str(text.as_str());
        if (parsed.is_err()) return Err(invalid_column());
        return Ok(rstd::move(parsed).unwrap());
    }
    static auto write(::sqlito::Statement& stmt, usize slot, const rstd::json::Value& value)
        -> Result<empty, ::sqlito::Error> {
        auto text = rstd::json::to_string(value);
        return Codec<String>::write(stmt, slot, text);
    }
};
