module;
#include <rstd/macro.hpp>
export module qcm.backend:resource.types;
export import :resource.format;
export import :resource.policy;
export import rstd;
export import sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;
using rstd::bytes::Bytes;
using rstd::path::PathBuf;
using rstd::sync::Arc;
using rstd::sync::Weak;
using rstd::sync::atomic::Ordering;

export namespace qcm::backend::resource_cache
{
template<class T>
using DbResult = Result<T, Error>;
template<class T>
using StorageFuture = rstd::async::coro<DbResult<T>>;
inline constexpr u64 LEASE_SECONDS { 120 };
inline constexpr u64 PACK_BYTES { 16 * 1024 * 1024 };
inline constexpr u64 MAX_OBJECT = PACK_BYTES - u64(64);

auto encode_kind(ResourceKind kind) -> DbResult<i64> {
    if (kind == ResourceKind::Image) return Ok(i64());
    if (kind == ResourceKind::Audio) return Ok(i64(1));
    return Err(Error::state("invalid resource kind"_str));
}
auto decode_kind(i64 kind) -> DbResult<ResourceKind> {
    if (kind == i64()) return Ok(ResourceKind::Image);
    if (kind == i64(1)) return Ok(ResourceKind::Audio);
    return Err(Error::state("invalid stored resource kind"_str));
}
struct ResourceKindDefinition {
    static constexpr bool serializable   = true;
    static constexpr bool deserializable = true;

    template<class S>
    static auto serialize(S& sink, ResourceKind kind) -> typename S::result_type {
        auto encoded = encode_kind(kind);
        if (encoded.is_err()) return Err(sink.invalid_value("invalid resource kind"_str));
        return sink.serialize_i64(*encoded);
    }
    template<class D>
    static auto deserialize(D& source) -> Result<ResourceKind, typename D::error_type> {
        auto kind = decode_kind(rstd_try(source.deserialize_i64()));
        if (kind.is_err()) return Err(source.invalid_value("invalid stored resource kind"_str));
        return Ok(*kind);
    }
};
constexpr auto rstd_serde_definition(ResourceKind*) { return ResourceKindDefinition {}; }

class ResourceHeaders {
    Headers headers_;

public:
    ResourceHeaders() = default;
    explicit ResourceHeaders(Headers headers): headers_(rstd::move(headers)) {}
    auto headers() const -> const Headers& { return headers_; }
    auto into_headers() && -> Headers { return rstd::move(headers_); }
};
struct ResourceHeadersDefinition {
    static constexpr bool serializable   = true;
    static constexpr bool deserializable = true;

    template<class S>
    static auto serialize(S& sink, const ResourceHeaders& value) -> typename S::result_type {
        auto output = rstd_try(sink.begin_sequence(value.headers().len()));
        for (const auto& header : value.headers()) {
            auto text = rstd::str_::from_utf8(header.value.as_slice());
            if (text.is_err()) return Err(sink.invalid_value("non-UTF-8 resource header"_str));
            Vec<ref<str>> pair;
            pair.push(header.name.as_str());
            pair.push(rstd::move(text).unwrap());
            rstd_try(output.element(pair));
        }
        return output.end();
    }
    template<class D>
    static auto deserialize(D& source) -> Result<ResourceHeaders, typename D::error_type> {
        using HeaderPair  = rstd::tuple<String, String>;
        using HeaderPairs = Vec<HeaderPair>;
        auto    values    = rstd_try(rstd::serde::deserialize<HeaderPairs>(source));
        Headers headers;
        for (const auto& pair : values) {
            auto added = headers.add(rstd::get<0>(pair).as_str(), rstd::get<1>(pair).as_str());
            if (added.is_err())
                return Err(source.invalid_value("invalid resource header value"_str));
        }
        return Ok(ResourceHeaders { rstd::move(headers) });
    }
};
constexpr auto rstd_serde_definition(ResourceHeaders*) { return ResourceHeadersDefinition {}; }

struct Metadata {
    String       key;
    i64          provider {};
    ResourceKind kind { ResourceKind::Image };
    String       generation;
    Geometry     geometry;
    Headers      headers;
    Policy       policy;
    String       target;
    bool         ranges {};
    Layout       layout;
    auto         clone() const -> Metadata;
};
struct Lease {
    rstd::sync::atomic::Atomic<bool> valid { true };
};
struct Version {
    i64        id {};
    Metadata   meta;
    Arc<Lease> lease;
    auto       is_valid() const -> bool { return lease->valid.load(Ordering::Acquire); }
    auto       clone() const -> Version { return { id, meta.clone(), lease.clone() }; }
};
struct ObjectMetadata {
    String  key, group;
    i64     provider {};
    u64     length {};
    Headers headers;
    Policy  policy;
    String  target, effective_url;
    auto    clone() const -> ObjectMetadata;
};

auto Metadata::clone() const -> Metadata {
    return { key.clone(), provider,        kind,           generation.clone(),
             geometry,    headers.clone(), policy.clone(), target.clone(),
             ranges,      layout };
}
auto ObjectMetadata::clone() const -> ObjectMetadata {
    return { key.clone(),     group.clone(),  provider,       length,
             headers.clone(), policy.clone(), target.clone(), effective_url.clone() };
}
struct Object {
    ObjectMetadata meta;
    Bytes          bytes;
};
struct Window {
    Vec<Arc<empty>> pins;
};
struct CommitOutcome {
    bool complete {}, newly_complete {};
};
using Blocks = Vec<rstd::tuple<u64, Bytes>>;
} // namespace qcm::backend::resource_cache
