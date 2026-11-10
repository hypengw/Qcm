module;
#include <rstd/macro.hpp>
export module qcm.backend:db.provider.schema;
export import liorm.sqlito;
import :datetime;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
struct ProviderEditTime {
    i64 millis;
};
} // namespace qcm::backend

export template<>
struct liorm::sqlito::Codec<qcm::backend::ProviderEditTime> {
    static constexpr auto parameter_type() -> liorm::query::ParameterType {
        return Codec<i64>::parameter_type();
    }
    static auto read(::sqlito::Statement& stmt, usize column)
        -> Result<qcm::backend::ProviderEditTime, ::sqlito::Error> {
        auto type = rstd_try(stmt.column_type(column));
        if (type == ::sqlito::ColumnType::Integer)
            return Ok(qcm::backend::ProviderEditTime { rstd_try(Codec<i64>::read(stmt, column)) });
        auto text = rstd_try(Codec<String>::read(stmt, column));
        auto time = qcm::backend::parse_database_datetime(text.as_str());
        if (time.is_none()) return Err(invalid_column());
        return Ok(qcm::backend::ProviderEditTime { time->seconds * i64(1000) +
                                                   i64(time->nanoseconds / 1000000U) });
    }
    static auto write(::sqlito::Statement& stmt, usize slot,
                      const qcm::backend::ProviderEditTime& time)
        -> Result<empty, ::sqlito::Error> {
        return Codec<i64>::write(stmt, slot, time.millis);
    }
};

export namespace qcm::backend
{
struct [[pmacro::derive("liorm::table"),
         pmacro::helper("liorm::table", name("provider"), primary("provider_pk"))]] ProviderRecord {
    [[pmacro::helper("liorm::table", storage(integer), primary)]]
    i64 provider_id;
    [[pmacro::helper("liorm::table", storage(text))]]
    String name;
    [[pmacro::helper("liorm::table", name("type"), storage(text))]]
    String type_name;
    [[pmacro::helper("liorm::table", storage(text))]]
    String base_url;
    [[pmacro::helper("liorm::table", storage(text), nullable)]]
    Option<String> auth_method;
    [[pmacro::helper("liorm::table", storage(text))]]
    String cookie;
    [[pmacro::helper("liorm::table", storage(text))]]
    String custom;
    [[pmacro::helper("liorm::table", storage(text))]]
    String session_key;
    [[pmacro::helper("liorm::table", storage(integer))]]
    ProviderEditTime edit_time;
};
struct [[pmacro::derive("liorm::table"), pmacro::helper("liorm::table", name("provider_settings"),
                                                        primary("provider_settings_pk"))]]
ProviderSettingsRecord {
    [[pmacro::helper("liorm::table", storage(integer), primary)]]
    i64 provider_id;
    [[pmacro::helper("liorm::table", storage(integer))]]
    i64 revision;
    [[pmacro::helper("liorm::table", storage(text))]]
    String config;
};
inline constexpr auto provider_tables = liorm::schema::define([](liorm::schema::SchemaBuilder& b) {
    auto provider = liorm::schema::register_table<ProviderRecord>(b);
    auto settings = liorm::schema::register_table<ProviderSettingsRecord>(b);
    b.foreign_key("settings_provider"_str,
                  { settings.column<&ProviderSettingsRecord::provider_id>() },
                  provider.primary_key);
});
} // namespace qcm::backend
