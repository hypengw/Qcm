module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:resource.policy;
export import lihttpto;
export import rstd.json;
using lihttpto::Headers;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend::resource_cache
{
auto now() -> u64 {
    auto elapsed =
        rstd::time::SystemTime::now().duration_since(rstd::time::SystemTime::unix_epoch());
    return elapsed.is_ok() ? elapsed->as_secs() : u64();
}
} // namespace qcm::backend::resource_cache

namespace qcm::backend::resource_cache::policy_detail
{
auto equal_ascii(ref<str> left, ref<str> right) -> bool {
    if (left.len() != right.len()) return false;
    for (usize index {}; index < left.len(); ++index) {
        auto a = left.as_bytes()[index].to_primitive();
        auto b = right.as_bytes()[index].to_primitive();
        if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
        if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
        if (a != b) return false;
    }
    return true;
}
auto text(const lihttpto::HeaderValue& field) -> Option<ref<str>> {
    // Rust HeaderValue::to_str rejects non-ASCII even when it is valid UTF-8.
    for (auto byte : field.as_slice())
        if (byte != u8('\t') && (byte < u8(32) || byte >= u8(127))) return None();
    auto parsed = field.to_str();
    return parsed.is_ok() ? Some(*parsed) : None();
}
auto unsigned_value(ref<str> text) -> Option<u64> {
    auto parsed = rstd::from_str<u64>(text);
    return parsed.is_ok() ? Some(*parsed) : None();
}
auto unsigned_value(const Option<String>& text, u64 fallback) -> u64 {
    return text.is_some() ? unsigned_value(text->as_str()).unwrap_or(fallback) : fallback;
}
template<class F>
auto parts(ref<str> text, F consume) -> void {
    for (;;) {
        auto split = text.split_once(","_str);
        if (split.is_none()) {
            consume(text);
            return;
        }
        consume(rstd::get<0>(*split));
        text = rstd::get<1>(*split);
    }
}
struct Directive {
    String         name;
    Option<String> argument;
};
auto directives(const Headers& headers) -> Vec<Directive> {
    Vec<Directive> result;
    for (const auto& field : headers) {
        if (! field.name.matches("cache-control"_str)) continue;
        auto parsed = text(field.value);
        if (parsed.is_none()) {
            result.push({ String::make("no-store"_str), None() });
            continue;
        }
        auto  input = *parsed;
        bool  quoted {};
        usize start {};
        for (usize index {}; index <= input.len(); ++index) {
            auto byte = index == input.len() ? u8(',') : input.as_bytes()[index];
            if (byte == u8('"')) quoted = ! quoted;
            if (byte != u8(',') || quoted) continue;
            auto part  = input.get(start, index).unwrap().trim_ascii();
            start      = index + usize(1);
            auto split = part.split_once("="_str);
            auto name  = split.is_some() ? rstd::get<0>(*split) : part;
            if (name.is_empty()) continue;
            Option<String> argument;
            if (split.is_some()) {
                auto  value = rstd::get<1>(*split).trim_ascii();
                usize begin {}, end = value.len();
                while (begin < end && value.as_bytes()[begin] == u8('"')) ++begin;
                while (end > begin && value.as_bytes()[end - usize(1)] == u8('"')) --end;
                argument = Some(String::make(value.get(begin, end).unwrap()));
            }
            auto lower = String::make(name.trim_ascii());
            lower.as_mut_str().make_ascii_lowercase();
            result.push({ rstd::move(lower), rstd::move(argument) });
        }
        if (quoted) result.push({ String::make("no-store"_str), None() });
    }
    return result;
}
auto has(const Vec<Directive>& directives, ref<str> name) -> bool {
    for (const auto& directive : directives)
        if (directive.name == name) return true;
    return false;
}
auto strong_etag(ref<str> value) -> bool {
    if (value.len() < usize(2) || ! value.starts_with("\""_str) || ! value.ends_with("\""_str))
        return false;
    for (usize index(1); index < value.len() - usize(1); ++index) {
        auto byte = value.as_bytes()[index];
        if (byte != u8(0x21) && ! (byte >= u8(0x23) && byte <= u8(0x7e)) && byte < u8(0x80))
            return false;
    }
    return true;
}
auto without_weak_prefix(ref<str> value) -> ref<str> {
    while (value.starts_with("W/"_str)) value = value.get(usize(2), value.len()).unwrap();
    return value;
}
} // namespace qcm::backend::resource_cache::policy_detail

export namespace qcm::backend::resource_cache
{
auto value(const Headers& headers, ref<str> name) -> Option<ref<str>> {
    auto field = headers.get(name);
    return field.is_some() ? policy_detail::text(**field) : None();
}
auto restore(const Headers& headers) -> Headers { return headers.clone(); }
auto forwarded(const Headers& headers) -> Headers {
    Vec<String> connection;
    if (auto field = value(headers, "connection"_str))
        policy_detail::parts(*field, [&](ref<str> part) {
            auto lower = String::make(part.trim_ascii());
            lower.as_mut_str().make_ascii_lowercase();
            connection.push(rstd::move(lower));
        });
    auto result = headers.clone();
    for (auto name : array<ref<str>, 10> { "connection"_str,
                                           "proxy-connection"_str,
                                           "keep-alive"_str,
                                           "proxy-authenticate"_str,
                                           "proxy-authorization"_str,
                                           "te"_str,
                                           "trailer"_str,
                                           "transfer-encoding"_str,
                                           "upgrade"_str,
                                           "set-cookie"_str })
        (void)result.remove(name);
    for (const auto& name : connection) (void)result.remove(name.as_str());
    return result;
}
auto saved(const Headers& headers) -> Headers {
    auto    filtered = forwarded(headers);
    Headers result;
    for (const auto& field : filtered)
        if (! field.name.matches("content-length"_str) &&
            ! field.name.matches("content-range"_str) && policy_detail::text(field.value).is_some())
            result.push(field.clone());
    return result;
}
} // namespace qcm::backend::resource_cache

namespace qcm::backend::resource_cache::policy_detail
{
auto date(const Headers& headers, ref<str> name) -> Option<u64> {
    auto text = value(headers, name);
    if (text.is_none()) return None();
    auto parsed = lihttpto::http_date(*text);
    if (parsed.is_none() || *parsed < i64()) return None();
    return Some(u64(parsed->to_primitive()));
}
} // namespace qcm::backend::resource_cache::policy_detail

export namespace qcm::backend::resource_cache
{
[[pmacro::invoke(
    "rstd::Enum",
    [[derive(Debug, Clone, "serde::derive")]] enum RangeValidator {
        StrongETag(String),
        [[helper("serde::derive", ignore_unknown)]] LastModified { value: String, date: u64 },
    } members {
    public:
        auto value() const& -> ref<str>;
        auto value() const&& -> ref<str> = delete;
        auto matches(const Headers& headers) const -> bool;
    })]];

inline auto RangeValidator::value() const& -> ref<str> {
    RSTD_MATCH(*this) {
        RSTD_CASE(StrongETag, value) { return value.as_str(); }
        RSTD_CASE_PAYLOAD(LastModified, modified) { return modified.value.as_str(); }
    }
}
inline auto RangeValidator::matches(const Headers& headers) const -> bool {
    RSTD_MATCH(*this) {
        RSTD_CASE(StrongETag, expected) {
            auto field = resource_cache::value(headers, "etag"_str);
            return field.is_some() && *field == expected.as_str();
        }
        RSTD_CASE_PAYLOAD(LastModified, modified) {
            if (headers.contains("etag"_str)) return false;
            auto field = resource_cache::value(headers, "last-modified"_str);
            return field.is_some() && *field == modified.value.as_str();
        }
    }
}

struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", ignore_unknown)]] Policy {
    u64  stored {};
    u64  initial_age {};
    u64  lifetime {};
    bool no_cache {};
    bool store {};
    [[pmacro::helper("serde::derive", default)]]
    Option<String> etag;
    [[pmacro::helper("serde::derive", default)]]
    Option<String> modified;
    [[pmacro::helper("serde::derive", default)]]
    Option<RangeValidator> range_validator;

    auto        clone() const -> Policy;
    static auto from_response(const Headers& headers, u64 sent, u64 received) -> Policy;
    auto        age() const -> u64;
    auto        fresh(const Headers& request) const -> bool;
    auto        range_value() const -> Option<ref<str>>;
    auto        matches_range(const Headers& headers) const -> bool;
    auto        conditional() const -> Headers;
    auto        to_json() const -> rstd::json::Value;
    static auto from_json(const rstd::json::Value&) -> Option<Policy>;
    static auto from_json(ref<str>) -> Option<Policy>;
};

auto Policy::clone() const -> Policy {
    return { stored, initial_age,  lifetime,         no_cache,
             store,  etag.clone(), modified.clone(), range_validator.clone() };
}
auto Policy::from_response(const Headers& headers, u64 sent, u64 received) -> Policy {
    using namespace policy_detail;
    auto        ds = directives(headers);
    usize       age_count {};
    Option<u64> max_age;
    for (const auto& directive : ds)
        if (directive.name == "max-age"_str) {
            ++age_count;
            if (directive.argument.is_some())
                max_age = unsigned_value(directive.argument->as_str());
        }
    auto   response_date = date(headers, "date"_str).unwrap_or(received);
    Policy result;
    result.stored = received;
    result.lifetime =
        age_count != usize()
            ? (age_count == usize(1) ? max_age.unwrap_or(u64()) : u64())
            : date(headers, "expires"_str).unwrap_or(response_date).saturating_sub(response_date);
    auto age_header = value(headers, "age"_str);
    auto age_value  = age_header.is_some() ? unsigned_value(*age_header).unwrap_or(u64()) : u64();
    result.initial_age = rstd::cmp::max(received.saturating_sub(response_date),
                                        age_value.saturating_add(received.saturating_sub(sent)));
    result.no_cache    = has(ds, "no-cache"_str);
    result.store       = ! has(ds, "no-store"_str) && ! has(ds, "must-understand"_str);
    for (const auto& field : headers) {
        auto parsed = text(field.value);
        if (parsed.is_none()) result.store = false;
        if (field.name.matches("vary"_str) && parsed.is_some())
            parts(*parsed, [&](ref<str> part) {
                if (part.trim_ascii() == "*"_str) result.store = false;
            });
    }
    auto encoding = value(headers, "content-encoding"_str);
    if (encoding.is_some() && ! equal_ascii(*encoding, "identity"_str)) result.store = false;
    auto tag           = value(headers, "etag"_str);
    auto last_modified = value(headers, "last-modified"_str);
    if (tag.is_some()) {
        result.etag = Some(String::make(*tag));
        if (strong_etag(*tag))
            result.range_validator = Some(RangeValidator::StrongETag(String::make(*tag)));
    } else {
        auto modified_date = date(headers, "last-modified"_str);
        auto original_date = date(headers, "date"_str);
        if (modified_date.is_some() && original_date.is_some() && last_modified.is_some() &&
            original_date->saturating_sub(*modified_date) >= u64(60))
            result.range_validator =
                Some(RangeValidator::LastModified(String::make(*last_modified), *original_date));
    }
    if (last_modified.is_some()) result.modified = Some(String::make(*last_modified));
    return result;
}
auto Policy::age() const -> u64 {
    if (now() < stored) return u64::MAX;
    return initial_age.saturating_add(now().saturating_sub(stored));
}
auto Policy::fresh(const Headers& request) const -> bool {
    auto pragma = value(request, "pragma"_str);
    if (! request.contains("cache-control"_str) && pragma.is_some() &&
        policy_detail::equal_ascii(*pragma, "no-cache"_str))
        return false;
    auto ds = policy_detail::directives(request);
    if (no_cache || policy_detail::has(ds, "no-cache"_str) ||
        policy_detail::has(ds, "no-store"_str))
        return false;
    auto allowed_lifetime = lifetime;
    for (const auto& directive : ds) {
        if (directive.name == "max-age"_str)
            allowed_lifetime = rstd::cmp::min(
                allowed_lifetime, policy_detail::unsigned_value(directive.argument, u64()));
        if (directive.name == "min-fresh"_str &&
            policy_detail::unsigned_value(directive.argument, u64::MAX).saturating_add(age()) >=
                allowed_lifetime)
            return false;
    }
    return age() < allowed_lifetime;
}
auto Policy::range_value() const -> Option<ref<str>> {
    if (range_validator.is_some()) return Some(range_validator->value());
    if (etag.is_some() && policy_detail::strong_etag(etag->as_str())) return Some(etag->as_str());
    return None();
}
auto Policy::matches_range(const Headers& headers) const -> bool {
    if (range_validator.is_some()) return range_validator->matches(headers);
    auto expected = range_value();
    auto actual   = value(headers, "etag"_str);
    return expected.is_some() && actual.is_some() && *expected == *actual;
}
auto Policy::conditional() const -> Headers {
    Headers result;
    if (etag.is_some())
        (void)result.add("if-none-match"_str, etag->as_str());
    else if (modified.is_some())
        (void)result.add("if-modified-since"_str, modified->as_str());
    return result;
}
auto Policy::to_json() const -> rstd::json::Value { return rstd::json::to_value(*this).unwrap(); }
auto Policy::from_json(const rstd::json::Value& object) -> Option<Policy> {
    return rstd::json::decode_value<Policy>(object).ok();
}
auto Policy::from_json(ref<str> text) -> Option<Policy> {
    return rstd::json::decode<Policy>(text).ok();
}

auto request_no_store(const Headers& headers) -> bool {
    return policy_detail::has(policy_detail::directives(headers), "no-store"_str);
}
auto only_cached(const Headers& headers) -> bool {
    return policy_detail::has(policy_detail::directives(headers), "only-if-cached"_str);
}
auto merge_validation(const Headers& old, const Headers& response) -> Headers {
    auto merged = restore(old);
    auto fields = saved(response);
    for (const auto& field : fields) {
        auto name = field.name.as_str();
        (void)merged.remove(name);
        for (const auto& updated : response)
            if (updated.name.matches(name)) merged.push(updated.clone());
    }
    (void)merged.remove("age"_str);
    if (! response.contains("date"_str)) {
        auto date =
            lihttpto::format_http_date(rstd::time::SystemTime::now().as_unix_time().seconds);
        if (date.is_some()) (void)merged.set("date"_str, date->as_str());
    }
    if (auto age = response.get("age"_str)) (void)merged.set("age"_str, (*age)->clone());
    return merged;
}
auto not_modified(const Policy& policy, const Headers& request) -> bool {
    if (auto tags = value(request, "if-none-match"_str)) {
        bool matched {};
        policy_detail::parts(*tags, [&](ref<str> tag) {
            tag = tag.trim_ascii();
            if (tag == "*"_str || (policy.etag.is_some() &&
                                   policy_detail::without_weak_prefix(tag) ==
                                       policy_detail::without_weak_prefix(policy.etag->as_str())))
                matched = true;
        });
        return matched;
    }
    auto since = policy_detail::date(request, "if-modified-since"_str);
    if (since.is_some() && policy.modified.is_some()) {
        auto modified = lihttpto::http_date(policy.modified->as_str());
        if (modified.is_some()) return *modified < i64() || u64(modified->to_primitive()) <= *since;
    }
    return false;
}
} // namespace qcm::backend::resource_cache
