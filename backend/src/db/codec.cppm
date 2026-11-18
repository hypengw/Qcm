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
        -> Result<qcm::backend::Timestamp, ::sqlito::Error>;
    static auto write(::sqlito::Statement& stmt, usize slot, qcm::backend::Timestamp value)
        -> Result<empty, ::sqlito::Error>;
};

export template<>
struct liorm::sqlito::Codec<qcm::backend::DatabaseDateTime> {
    static constexpr auto parameter_type() -> liorm::query::ParameterType {
        return Codec<String>::parameter_type();
    }
    static auto read(::sqlito::Statement& stmt, usize column)
        -> Result<qcm::backend::DatabaseDateTime, ::sqlito::Error>;
    static auto write(::sqlito::Statement& stmt, usize slot,
                      const qcm::backend::DatabaseDateTime& value)
        -> Result<empty, ::sqlito::Error>;
};

export template<>
struct liorm::sqlito::Codec<rstd::json::Value> {
    static constexpr auto parameter_type() -> liorm::query::ParameterType {
        return Codec<String>::parameter_type();
    }
    static auto read(::sqlito::Statement& stmt, usize column)
        -> Result<rstd::json::Value, ::sqlito::Error>;
    static auto write(::sqlito::Statement& stmt, usize slot, const rstd::json::Value& value)
        -> Result<empty, ::sqlito::Error>;
};
