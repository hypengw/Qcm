export module qcm.backend:model.subtitle;
export import rstd;
export import rstd.serde;
import rstd.parse.alloc;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", rename("SubtitleItem"), ignore_unknown)]] SubtitleItem {
    [[pmacro::helper("serde::derive", default)]]
    Option<i64> start;
    [[pmacro::helper("serde::derive", default)]]
    Option<i64> end;
    [[pmacro::helper("serde::derive", default)]]
    Option<String> text;
};
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", rename("Subtitle"), ignore_unknown)]] SubtitleData {
    Vec<SubtitleItem> items;
};
enum class SubtitleParseErrorKind
{
    InvalidLineEnding,
    Stalled,
    Capacity
};
struct SubtitleParseError {
    SubtitleParseErrorKind kind;
    usize                  offset;
};
} // namespace qcm::backend

namespace qcm::backend::lrc_detail
{
namespace p = rstd::parse;

struct Adapter {
    using error_type = SubtitleParseError;

    auto expected(p::RuleId, p::Span span) const -> SubtitleParseError {
        return { SubtitleParseErrorKind::InvalidLineEnding, span.begin };
    }
    auto stalled(p::RuleId, p::Span span) const -> SubtitleParseError {
        return { SubtitleParseErrorKind::Stalled, span.begin };
    }
    auto capacity(p::RuleId, p::Span span) const -> SubtitleParseError {
        return { SubtitleParseErrorKind::Capacity, span.begin };
    }
};
using Driver = p::Driver<u8, Adapter, p::NoopObserver>;

enum class MetadataKind
{
    Artist,
    Album,
    Title,
    Lyricist,
    Author,
    Length,
    Offset,
    Application,
    AppVersion,
    Unknown
};
struct Metadata {
    MetadataKind kind;
    p::Span      value;
    Option<i64>  offset;
};
struct Tag {
    Vec<i64>         timestamps;
    p::Span          text;
    Option<Metadata> metadata;
};
struct Content {
    Option<Tag> tag;
};

auto trim_end(const p::TextCursor& cursor, p::Span span) -> p::Span {
    auto  text = cursor.text(span).unwrap();
    usize length {}, trimmed {};
    for (auto ch : text.chars()) {
        length += ch.len_utf8();
        if (! ch.is_whitespace()) trimmed = length;
    }
    return { span.begin, span.begin + trimmed };
}

constexpr auto decimal_max = (u128(1) << u64(96)) - u128(1);

auto power_of_ten(usize exponent) -> u128 {
    auto result = u128(1);
    for (usize index {}; index < exponent; ++index) result *= u128(10);
    return result;
}

auto rounded_quotient(u128 value, u128 divisor) -> u128 {
    auto quotient  = value / divisor;
    auto remainder = value % divisor;
    if (remainder > divisor / u128(2) ||
        (divisor > u128(1) && remainder == divisor / u128(2) && quotient % u128(2) != u128()))
        quotient += u128(1);
    return quotient;
}

auto coefficient(const p::TextCursor& cursor, p::Span span) -> Option<u128> {
    auto value = rstd::from_str<u128>(cursor.text(span).unwrap());
    if (value.is_err() || *value > decimal_max) return None();
    return Some(*value);
}

auto timestamp(const p::TextCursor& cursor, p::Span minutes, p::Span seconds, p::Span fraction)
    -> Option<i64> {
    if (fraction.len() > usize(28)) return None();
    auto minute = coefficient(cursor, minutes);
    auto second = coefficient(cursor, seconds);
    auto part   = coefficient(cursor, fraction);
    if (minute.is_none() || second.is_none() || part.is_none()) return None();
    auto whole = *minute * u128(60) + *second;
    if (whole > u128((i64::MAX / i64(1000)).to_primitive())) return None();

    // Decimal addition rounds to the largest scale whose coefficient fits in 96 bits.
    auto scale = fraction.len();
    u128 sum;
    for (;;) {
        auto factor = power_of_ten(scale);
        if (whole <= decimal_max / factor) {
            auto divisor = power_of_ten(fraction.len() - scale);
            auto base    = whole * factor;
            auto digits  = *part / divisor;
            if (digits <= decimal_max - base) {
                sum            = base + digits;
                auto remainder = *part % divisor;
                if (remainder > divisor / u128(2) ||
                    (divisor > u128(1) && remainder == divisor / u128(2) &&
                     sum % u128(2) != u128()))
                    sum += u128(1);
                if (sum <= decimal_max) break;
            }
        }
        if (scale == usize()) return None();
        --scale;
    }

    auto product = sum * u128(1000);
    for (usize dropped {}; dropped <= scale; ++dropped) {
        auto scaled = rounded_quotient(product, power_of_ten(dropped));
        if (scaled > decimal_max) continue;
        auto millis = scaled / power_of_ten(scale - dropped);
        if (millis > u128(i64::MAX.to_primitive())) return None();
        return Some(i64(static_cast<rstd::int64_t>(millis.to_primitive())));
    }
    return None();
}

struct NumberRule {
    using value_type               = p::Span;
    static constexpr bool nullable = false;

    auto id() const -> p::RuleId { return p::RuleId("LRC number"_str); }
    auto match(Driver& driver) -> p::Match<value_type, SubtitleParseError> {
        auto& cursor = driver.cursor();
        p::skip_while(cursor, p::ascii::blank);
        auto digits = p::consume_while_one(cursor, p::ascii::digit);
        p::skip_while(cursor, p::ascii::blank);
        return Ok(digits);
    }
};

struct TimestampRule {
    using value_type               = i64;
    static constexpr bool nullable = false;

    auto id() const -> p::RuleId { return p::RuleId("LRC timestamp"_str); }
    auto match(Driver& driver) -> p::Match<value_type, SubtitleParseError> {
        auto&      cursor = driver.cursor();
        NumberRule number;
        auto       minutes = driver.apply(number).unwrap();
        if (minutes.is_none() || p::consume_literal(cursor, ":"_str).is_none())
            return Ok<Option<i64>>(None());
        auto seconds = driver.apply(number).unwrap();
        if (seconds.is_none() ||
            p::consume_while_one(cursor, p::one_of(u8(':'), u8('.'))).is_none())
            return Ok<Option<i64>>(None());
        auto fraction = driver.apply(number).unwrap();
        if (fraction.is_none()) return Ok<Option<i64>>(None());
        return Ok(timestamp(cursor, *minutes, *seconds, *fraction));
    }
};

auto line_text(p::TextCursor& cursor) -> Option<p::Span> {
    auto span = p::consume_until(cursor, p::one_of(u8('\r'), u8('\n')));
    auto next = cursor.peek();
    if (next.is_some() && next->get() == u8('\r')) {
        auto following = cursor.peek(usize(1));
        if (following.is_none() || following->get() != u8('\n')) return None();
    }
    return Some(span);
}

struct TimeTagRule {
    using value_type               = Tag;
    static constexpr bool nullable = false;

    auto id() const -> p::RuleId { return p::RuleId("LRC time tag"_str); }
    auto match(Driver& driver) -> p::Match<value_type, SubtitleParseError> {
        auto timestamps =
            p::repeat_one(p::RuleId("LRC timestamps"_str),
                          p::delimited(p::RuleId("LRC time brackets"_str),
                                       p::text(p::RuleId("LRC opening bracket"_str), "["_str),
                                       TimestampRule {},
                                       p::text(p::RuleId("LRC closing bracket"_str), "]"_str)));
        auto values = driver.apply(timestamps);
        if (values.is_err()) return Err(rstd::move(values).unwrap_err());
        auto optional = rstd::move(values).unwrap();
        if (optional.is_none()) return Ok<Option<Tag>>(None());
        p::skip_while(driver.cursor(), p::ascii::blank);
        auto text = line_text(driver.cursor());
        if (text.is_none()) return Ok<Option<Tag>>(None());
        return Ok(
            Some(Tag { rstd::move(optional).unwrap(), trim_end(driver.cursor(), *text), None() }));
    }
};

auto metadata_kind(ref<str> id) -> MetadataKind {
    if (id == "ar"_str) return MetadataKind::Artist;
    if (id == "al"_str) return MetadataKind::Album;
    if (id == "ti"_str) return MetadataKind::Title;
    if (id == "au"_str) return MetadataKind::Lyricist;
    if (id == "by"_str) return MetadataKind::Author;
    if (id == "length"_str) return MetadataKind::Length;
    if (id == "offset"_str) return MetadataKind::Offset;
    if (id == "re"_str) return MetadataKind::Application;
    if (id == "ve"_str) return MetadataKind::AppVersion;
    return MetadataKind::Unknown;
}

struct MetadataTagRule {
    using value_type               = Tag;
    static constexpr bool nullable = false;

    auto id() const -> p::RuleId { return p::RuleId("LRC metadata"_str); }
    auto match(Driver& driver) -> p::Match<value_type, SubtitleParseError> {
        auto& cursor = driver.cursor();
        if (p::consume_literal(cursor, "["_str).is_none()) return Ok<Option<Tag>>(None());
        p::skip_while(cursor, p::ascii::blank);
        auto name = p::consume_while_one(cursor, p::ascii::alpha);
        p::skip_while(cursor, p::ascii::blank);
        if (name.is_none() || p::consume_literal(cursor, ":"_str).is_none())
            return Ok<Option<Tag>>(None());
        p::skip_while(cursor, p::ascii::blank);
        auto value = p::consume_while_one(cursor, p::not_(p::equal_to(u8(']'))));
        if (value.is_none() || p::consume_literal(cursor, "]"_str).is_none())
            return Ok<Option<Tag>>(None());
        auto metadata = Metadata { metadata_kind(cursor.text(*name).unwrap()),
                                   trim_end(cursor, *value),
                                   None() };
        if (metadata.kind == MetadataKind::Offset) {
            auto offset = rstd::from_str<i64>(cursor.text(metadata.value).unwrap());
            if (offset.is_err()) return Ok<Option<Tag>>(None());
            metadata.offset = Some(*offset);
        }
        return Ok(Some(Tag { {}, {}, Some(rstd::move(metadata)) }));
    }
};

struct TagRule {
    using value_type               = Tag;
    static constexpr bool nullable = false;

    auto id() const -> p::RuleId { return p::RuleId("LRC tag"_str); }
    auto match(Driver& driver) -> p::Match<value_type, SubtitleParseError> {
        p::skip_while(driver.cursor(), p::ascii::blank);
        auto rule = p::choice(id(), TimeTagRule {}, MetadataTagRule {});
        return driver.apply(rule);
    }
};

struct UnknownRule {
    using value_type               = Content;
    static constexpr bool nullable = true;

    auto id() const -> p::RuleId { return p::RuleId("LRC unknown line"_str); }
    auto match(Driver& driver) -> p::Match<value_type, SubtitleParseError> {
        if (line_text(driver.cursor()).is_none())
            return Err(SubtitleParseError { SubtitleParseErrorKind::InvalidLineEnding,
                                            driver.cursor().position() });
        return Ok(Some(Content { None() }));
    }
};

auto parse_tags(ref<str> input) -> Result<Vec<Tag>, SubtitleParseError> {
    Adapter         adapter;
    p::NoopObserver observer;
    Driver          driver(p::text_input(input), adapter, observer);

    auto     tagged  = p::map(p::RuleId("LRC tagged content"_str), TagRule {}, [](Tag tag) {
        return Content { Some(rstd::move(tag)) };
    });
    auto     content = p::choice(p::RuleId("LRC content"_str), rstd::move(tagged), UnknownRule {});
    auto     ending  = p::optional(p::RuleId("LRC optional line ending"_str),
                                   p::choice(p::RuleId("LRC line ending"_str),
                                             p::text(p::RuleId("LRC LF"_str), "\n"_str),
                                             p::text(p::RuleId("LRC CRLF"_str), "\r\n"_str)));
    Vec<Tag> tags;
    while (! driver.cursor().is_eof()) {
        auto before = driver.cursor().position();
        auto parsed = driver.apply(content);
        if (parsed.is_err()) return Err(rstd::move(parsed).unwrap_err());
        auto item = rstd::move(parsed).unwrap().unwrap();
        if (item.tag.is_some()) tags.push(rstd::move(item.tag).unwrap());
        auto newline = driver.apply(ending);
        if (newline.is_err()) return Err(rstd::move(newline).unwrap_err());
        if (driver.cursor().position() == before)
            return Err(SubtitleParseError { SubtitleParseErrorKind::Stalled, before });
    }
    return Ok(rstd::move(tags));
}
} // namespace qcm::backend::lrc_detail

export namespace qcm::backend
{
auto parse_lrc(ref<str> input) -> Result<SubtitleData, SubtitleParseError> {
    auto parsed = lrc_detail::parse_tags(input);
    if (parsed.is_err()) return Err(rstd::move(parsed).unwrap_err());
    auto         source = rstd::parse::text_input(input);
    SubtitleData result;
    for (auto& tag : *parsed) {
        if (tag.timestamps.is_empty() || tag.text.is_empty()) continue;
        SubtitleItem item;
        item.start = Some(tag.timestamps[usize()]);
        if (tag.timestamps.len() > usize(1)) item.end = Some(tag.timestamps[usize(1)]);
        item.text = Some(String::make(source.text(tag.text).unwrap()));
        result.items.push(rstd::move(item));
    }
    return Ok(rstd::move(result));
}
} // namespace qcm::backend

export namespace rstd
{
template<>
struct Impl<fmt::Display, qcm::backend::SubtitleParseError>
    : ImplBase<qcm::backend::SubtitleParseError> {
    auto fmt(fmt::Formatter& formatter) const -> bool {
        const auto& error = this->self();
        using Kind        = qcm::backend::SubtitleParseErrorKind;
        auto message      = error.kind == Kind::InvalidLineEnding ? "invalid LRC line ending"_str
                            : error.kind == Kind::Stalled ? "LRC parser made no progress"_str
                                                          : "LRC parser capacity exceeded"_str;
        return formatter.write_fmt(fmt::Arguments::make("{} at byte {}", message, error.offset));
    }
};
template<>
struct Impl<fmt::Debug, qcm::backend::SubtitleParseError>
    : Impl<fmt::Display, qcm::backend::SubtitleParseError> {};
template<>
struct Impl<error::Error, qcm::backend::SubtitleParseError>
    : DefaultInImpl<error::Error, qcm::backend::SubtitleParseError> {};
} // namespace rstd
