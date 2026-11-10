module;
#include <rstd/macro.hpp>
export module qcm.backend:datetime;
export import rstd;
export import rstd.serde;
import rstd.parse.core;
using namespace rstd::prelude;
using namespace rstd::literals;
using rstd::time::CalendarDate;
using rstd::time::DateTimeParseError;
using rstd::time::Month;
using rstd::time::ParsedDateTime;
using rstd::time::UtcOffset;

export namespace qcm::backend
{
struct DatabaseDateTime {
    i64      seconds;
    unsigned nanoseconds;
};
using UtcDateTime = DatabaseDateTime;

auto datetime_now() -> DatabaseDateTime {
    auto value = rstd::time::SystemTime::now().as_unix_time();
    return { value.seconds, value.nanoseconds.to_primitive() };
}

auto datetime_valid_seconds(long long seconds) -> bool {
    constexpr auto first =
        *rstd::time::days_from_calendar_date(i32(-262143), Month::January, u8(1)) * i64(86400);
    constexpr auto end =
        *rstd::time::days_from_calendar_date(i32(262143), Month::January, u8(1)) * i64(86400);
    return i64(seconds) >= first && i64(seconds) < end;
}
} // namespace qcm::backend

namespace qcm::backend
{
auto database_time(ParsedDateTime value) -> DatabaseDateTime {
    return { value.base_time.seconds,
             value.base_time.nanoseconds.to_primitive() + (value.leap_second ? 1000000000U : 0U) };
}

struct DateNumber {
    i32               value;
    rstd::parse::Span span;
};

class DatabaseDateTimeParser {
    rstd::parse::TextCursor cursor_;

    auto skip_space() -> void {
        auto text = cursor_.remaining_text().unwrap();
        for (auto ch : text.chars()) {
            if (! ch.is_whitespace()) break;
            (void)cursor_.advance(ch.len_utf8());
        }
    }
    auto number(usize width, bool whitespace = true) -> Result<DateNumber, DateTimeParseError> {
        if (whitespace) skip_space();
        auto start = cursor_.position();
        auto span  = rstd::parse::consume_while_one(cursor_, [&](u8 byte) {
            return cursor_.position() - start < width && rstd::parse::ascii::digit(byte);
        });
        if (span.is_none()) return Err(DateTimeParseError::syntax(cursor_.position()));
        auto value = rstd::from_str<i32>(cursor_.text(*span).unwrap());
        if (value.is_err()) return Err(DateTimeParseError::syntax(span->begin));
        return Ok(DateNumber { *value, *span });
    }
    auto consume(ref<str> text) -> bool {
        return rstd::parse::consume_literal(cursor_, text).is_some();
    }
    auto literal(ref<str> text) -> Result<empty, DateTimeParseError> {
        if (! consume(text)) return Err(DateTimeParseError::syntax(cursor_.furthest_position()));
        return Ok(empty {});
    }

public:
    explicit DatabaseDateTimeParser(ref<str> text): cursor_(rstd::parse::text_input(text)) {}

    auto parse() -> Result<ParsedDateTime, DateTimeParseError> {
        skip_space();
        auto year_start = cursor_.checkpoint();
        bool signed_year =
            rstd::parse::consume_if(cursor_, rstd::parse::one_of(u8('+'), u8('-'))).is_some();
        DateNumber year;
        if (signed_year) {
            if (rstd::parse::consume_while_one(cursor_, rstd::parse::ascii::digit).is_none())
                return Err(DateTimeParseError::syntax(cursor_.position()));
            auto value = rstd::from_str<i32>(cursor_.consumed_text(year_start).unwrap());
            if (value.is_err()) return Err(DateTimeParseError::syntax(cursor_.position()));
            year = { *value, cursor_.span_from(year_start) };
        } else {
            year = rstd_try(number(usize(4)));
        }
        if (year.value < i32(-262143) || year.value > i32(262142))
            return Err(DateTimeParseError::range());
        rstd_try(literal("-"_str));
        auto month = rstd_try(number(usize(2)));
        rstd_try(literal("-"_str));
        auto day             = rstd_try(number(usize(2)));
        auto separator       = cursor_.position();
        auto next            = cursor_.peek();
        bool space_separator = next.is_some() && next->get() == u8(' ');
        bool lower_t         = consume("t"_str);
        bool upper_t         = ! lower_t && consume("T"_str);
        auto hour            = rstd_try(number(usize(2)));
        rstd_try(literal(":"_str));
        auto       minute = rstd_try(number(usize(2)));
        DateNumber second { i32(), {} };
        bool       has_seconds = consume(":"_str);
        u32        nanos;
        if (has_seconds) {
            second = rstd_try(number(usize(2)));
            if (consume("."_str)) {
                auto fraction = rstd::parse::consume_while_one(cursor_, rstd::parse::ascii::digit);
                if (fraction.is_none()) return Err(DateTimeParseError::syntax(cursor_.position()));
                usize count;
                for (auto digit : cursor_.view(*fraction)) {
                    if (count == usize(9)) break;
                    nanos = nanos * u32(10) + u32((digit - u8('0')).to_primitive());
                    ++count;
                }
                while (count < usize(9)) {
                    nanos *= u32(10);
                    ++count;
                }
            }
        }
        bool      has_offset  = ! cursor_.is_eof();
        bool      lower_z     = false;
        bool      strict_zone = true;
        UtcOffset offset;
        if (! consume("Z"_str) && ! (lower_z = consume("z"_str))) {
            auto before = cursor_.checkpoint();
            skip_space();
            auto sign_position = cursor_.position();
            bool negative      = consume("-"_str) || consume("\xe2\x88\x92"_str);
            bool positive      = ! negative && consume("+"_str);
            if (negative || positive) {
                auto offset_hour = rstd_try(number(usize(2), false));
                if (offset_hour.span.len() != usize(2))
                    return Err(DateTimeParseError::syntax(cursor_.position()));
                auto separator_start = cursor_.position();
                while (true) {
                    if (consume(":"_str)) continue;
                    auto position = cursor_.position();
                    skip_space();
                    if (position == cursor_.position()) break;
                }
                auto offset_minute = rstd_try(number(usize(2), false));
                if (offset_minute.span.len() != usize(2) || offset_minute.value > i32(59))
                    return Err(DateTimeParseError::syntax(offset_minute.span.begin));
                strict_zone = cursor_.span_from(before).begin == sign_position &&
                              offset_minute.span.begin == separator_start + usize(1) &&
                              cursor_.view({ separator_start,
                                             separator_start + usize(1) })[usize()] == u8(':');
                if (offset_hour.value > i32(23)) {
                    if (lower_t) return Err(DateTimeParseError::range());
                } else {
                    auto value = offset_hour.value * i32(3600) + offset_minute.value * i32(60);
                    if (negative) value = -value;
                    offset = UtcOffset::from_whole_seconds(value).unwrap();
                }
            } else {
                cursor_.rewind(before);
            }
        }
        bool rfc_prefix = ! signed_year && year.span.begin == usize() &&
                          year.span.len() == usize(4) && month.span.begin == usize(5) &&
                          month.span.len() == usize(2) && day.span.begin == usize(8) &&
                          day.span.len() == usize(2) && separator == usize(10) &&
                          (lower_t || upper_t || space_separator) && hour.span.begin == usize(11) &&
                          hour.span.len() == usize(2) && minute.span.begin == usize(14) &&
                          minute.span.len() == usize(2) && second.span.begin == usize(17) &&
                          second.span.len() == usize(2);
        if (! cursor_.is_eof() || ((lower_t || lower_z) && (! has_offset || ! has_seconds ||
                                                            ! rfc_prefix || ! strict_zone)))
            return Err(DateTimeParseError::syntax(cursor_.position()));
        auto parsed_month = Month::from_number(u8(month.value.to_primitive()));
        if (parsed_month.is_err())
            return Err(DateTimeParseError::invalid(parsed_month.unwrap_err(), month.span.begin));
        CalendarDate date { year.value, *parsed_month, u8(day.value.to_primitive()) };
        auto parsed = rstd_try(rstd::time::make_parsed_datetime(date,
                                                                u8(hour.value.to_primitive()),
                                                                u8(minute.value.to_primitive()),
                                                                u8(second.value.to_primitive()),
                                                                nanos,
                                                                offset));
        if (! datetime_valid_seconds(parsed.base_time.seconds.to_primitive())) {
            if (lower_t || lower_z) return Err(DateTimeParseError::range());
            parsed = rstd_try(rstd::time::make_parsed_datetime(date,
                                                               u8(hour.value.to_primitive()),
                                                               u8(minute.value.to_primitive()),
                                                               u8(second.value.to_primitive()),
                                                               nanos));
            if (! datetime_valid_seconds(parsed.base_time.seconds.to_primitive()))
                return Err(DateTimeParseError::range());
        }
        return Ok(parsed);
    }
};
} // namespace qcm::backend

export namespace qcm::backend
{
auto parse_database_datetime(ref<str> text) -> Option<DatabaseDateTime> {
    auto parsed = DatabaseDateTimeParser(text).parse();
    if (parsed.is_err()) return None();
    return Some(database_time(*parsed));
}

auto parse_utc_datetime(ref<str> text) -> Option<UtcDateTime> {
    auto parsed = rstd::time::parse_rfc3339(text,
                                            { .allow_space_separator = false,
                                              .allow_unicode_minus   = false,
                                              .max_fractional_digits = usize(9) });
    if (parsed.is_err() || parsed->leap_second ||
        ! datetime_valid_seconds(parsed->base_time.seconds.to_primitive()))
        return None();
    return Some(database_time(*parsed));
}

auto encode_database_datetime(const DatabaseDateTime& value, bool use_z = false) -> Option<String> {
    if (! datetime_valid_seconds(value.seconds.to_primitive()) || value.nanoseconds >= 2000000000U)
        return None();
    bool leap    = value.nanoseconds >= 1000000000U;
    auto encoded = rstd::time::format_rfc3339(
        { { value.seconds, u32(value.nanoseconds - (leap ? 1000000000U : 0U)) }, leap }, use_z);
    if (encoded.is_err()) return None();
    return Some(rstd::move(*encoded));
}

auto format_utc_datetime(const UtcDateTime& value) -> Option<String> {
    constexpr auto first =
        *rstd::time::days_from_calendar_date(i32(), Month::January, u8(1)) * i64(86400);
    constexpr auto end =
        *rstd::time::days_from_calendar_date(i32(10000), Month::January, u8(1)) * i64(86400);
    if (value.seconds < first || value.seconds >= end || value.nanoseconds >= 1000000000U)
        return None();
    return encode_database_datetime(value, true);
}

struct DatabaseDateTimeDefinition {
    static constexpr bool serializable   = true;
    static constexpr bool deserializable = true;
    template<class S>
    static auto serialize(S& sink, const DatabaseDateTime& value) -> typename S::result_type {
        auto text = encode_database_datetime(value, true);
        if (text.is_none()) return Err(sink.invalid_value("invalid datetime"_str));
        return sink.serialize_string(text->as_str());
    }
    template<class D>
    static auto deserialize(D& source) -> Result<DatabaseDateTime, typename D::error_type> {
        auto text = rstd_try(source.deserialize_string());
        auto time = parse_database_datetime(text.as_str());
        if (time.is_none()) return Err(source.invalid_value("invalid datetime"_str));
        return Ok(*time);
    }
};
constexpr auto rstd_serde_definition(DatabaseDateTime*) { return DatabaseDateTimeDefinition {}; }
} // namespace qcm::backend
