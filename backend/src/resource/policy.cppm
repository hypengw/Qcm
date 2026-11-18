export module qcm.backend:resource.policy;
export import lihttpto;
export import rstd.json;
using lihttpto::Headers;
using namespace rstd::prelude;

export namespace qcm::backend::resource_cache
{
auto now() -> u64;
} // namespace qcm::backend::resource_cache

export namespace qcm::backend::resource_cache
{
auto value(const Headers& headers, ref<str> name) -> Option<ref<str>>;
auto restore(const Headers& headers) -> Headers;
auto forwarded(const Headers& headers) -> Headers;
auto saved(const Headers& headers) -> Headers;
} // namespace qcm::backend::resource_cache

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

auto request_no_store(const Headers& headers) -> bool;
auto only_cached(const Headers& headers) -> bool;
auto merge_validation(const Headers& old, const Headers& response) -> Headers;
auto not_modified(const Policy& policy, const Headers& request) -> bool;
} // namespace qcm::backend::resource_cache
