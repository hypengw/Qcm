module;
#include <rstd/macro.hpp>
module qcm.backend;
import :resource.storage;
import rstd.json;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend::resource_cache
{
namespace metadata_detail
{
struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", ignore_unknown)]]
MetadataWire {
    String          key;
    i64             provider;
    ResourceKind    kind;
    String          generation;
    Geometry        geometry;
    ResourceHeaders headers;
    Policy          policy;
    String          target;
    bool            ranges;
    [[pmacro::helper("serde::derive", default)]]
    Layout layout;

    static auto from(const Metadata&) -> MetadataWire;
    auto        into_metadata() && -> Metadata;
};
struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", ignore_unknown)]]
ObjectMetadataWire {
    String          key, group;
    i64             provider;
    u64             length;
    ResourceHeaders headers;
    Policy          policy;
    String          target, effective_url;

    static auto from(const ObjectMetadata&) -> ObjectMetadataWire;
    auto        into_metadata() && -> ObjectMetadata;
};

auto MetadataWire::from(const Metadata& meta) -> MetadataWire {
    return { meta.key.clone(),    meta.provider,
             meta.kind,           meta.generation.clone(),
             meta.geometry,       ResourceHeaders { meta.headers.clone() },
             meta.policy.clone(), meta.target.clone(),
             meta.ranges,         meta.layout };
}
auto MetadataWire::into_metadata() && -> Metadata {
    return { rstd::move(key),
             provider,
             kind,
             rstd::move(generation),
             geometry,
             rstd::move(headers).into_headers(),
             rstd::move(policy),
             rstd::move(target),
             ranges,
             layout };
}
auto ObjectMetadataWire::from(const ObjectMetadata& meta) -> ObjectMetadataWire {
    return { meta.key.clone(),
             meta.group.clone(),
             meta.provider,
             meta.length,
             ResourceHeaders { meta.headers.clone() },
             meta.policy.clone(),
             meta.target.clone(),
             meta.effective_url.clone() };
}
auto ObjectMetadataWire::into_metadata() && -> ObjectMetadata {
    return { rstd::move(key),
             rstd::move(group),
             provider,
             length,
             rstd::move(headers).into_headers(),
             rstd::move(policy),
             rstd::move(target),
             rstd::move(effective_url) };
}
} // namespace metadata_detail

auto encode_metadata(const Metadata& meta) -> DbResult<String> {
    return rstd::json::encode(metadata_detail::MetadataWire::from(meta)).map_err([](auto error) {
        return Error::state("invalid resource metadata"_str, rstd::move(error));
    });
}
auto decode_metadata(ref<str> input) -> DbResult<Metadata> {
    auto stored = rstd_try(
        rstd::json::decode_direct<metadata_detail::MetadataWire>(input).map_err([](auto error) {
            return Error::state("invalid resource metadata JSON"_str, rstd::move(error));
        }));
    return Ok(rstd::move(stored).into_metadata());
}
auto encode_object_metadata(const ObjectMetadata& meta) -> DbResult<String> {
    return rstd::json::encode(metadata_detail::ObjectMetadataWire::from(meta))
        .map_err([](auto error) {
            return Error::state("invalid object metadata"_str, rstd::move(error));
        });
}
auto decode_object_metadata(ref<str> input) -> DbResult<ObjectMetadata> {
    auto stored =
        rstd_try(rstd::json::decode_direct<metadata_detail::ObjectMetadataWire>(input).map_err(
            [](auto error) {
                return Error::state("invalid resource metadata JSON"_str, rstd::move(error));
            }));
    return Ok(rstd::move(stored).into_metadata());
}
} // namespace qcm::backend::resource_cache
