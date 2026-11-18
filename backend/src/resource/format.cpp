module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
module qcm.backend;
import :resource.format;
import licrypto;
using namespace rstd::prelude;
using namespace rstd::literals;
using rstd::tuple;
using rstd::bytes::Bytes;

namespace qcm::backend::resource_cache
{
namespace
{
auto format_error(ref<str> message) -> Error { return Error::state(message); }
} // namespace

auto hash(slice<u8> data) -> Result<array<u8, 32>, Error> {
    auto digest = licrypto::sha256(data);
    if (digest.is_err()) {
        return Err(Error::state("resource SHA-256 failed"_str, rstd::move(digest).unwrap_err()));
    }
    return Ok(rstd::move(*digest));
}

auto identity(slice<u8> data) -> Result<String, Error> {
    return Ok(licrypto::sha256_hex(rstd_try(hash(data))));
}

auto Geometry::make(u64 length) -> Result<Geometry, Error> {
    Geometry value { length, MIN_BLOCK };
    while (value.count() > u64(8192) && value.block < MAX_BLOCK) value.block *= u64(2);
    if (value.count() > u64(1000000))
        return Err(format_error("resource index exceeds memory limit"_str));
    return Ok(value);
}

auto Geometry::range(u64 index) const -> Result<Range, Error> {
    if (index >= count()) return Err(format_error("block out of bounds"_str));
    auto start     = index * block;
    auto remaining = length - start;
    return Ok(Range { start, start + (remaining < block ? remaining : block) });
}

auto Geometry::offset(u32 slot) const -> Result<u64, Error> {
    auto position = u64(slot.to_primitive()).checked_mul(block);
    if (position.is_none()) return Err(format_error("slot overflow"_str));
    auto offset = HEADER_LEN.checked_add(*position);
    if (offset.is_none()) return Err(format_error("offset overflow"_str));
    return Ok(*offset);
}

auto Geometry::header(ref<str> generation) const -> Result<array<u8, 64>, Error> {
    array<u8, 64> bytes {};
    auto          magic = "QCMRSC01"_str.as_bytes();
    for (usize i {}; i < usize(8); ++i) bytes[i] = magic[i];
    auto length_bytes = length.to_le_bytes();
    auto block_bytes  = block.to_le_bytes();
    for (usize i {}; i < usize(8); ++i) {
        bytes[usize(8) + i]  = length_bytes[i];
        bytes[usize(16) + i] = block_bytes[i];
    }
    auto digest = rstd_try(hash(generation.as_bytes()));
    for (usize i {}; i < usize(32); ++i) bytes[usize(24) + i] = digest[i];
    return Ok(bytes);
}

auto Layout::for_resource(ResourceKind kind, u64 length) -> Layout {
    return kind == ResourceKind::Audio && length > SEGMENT_BYTES ? Segments(SEGMENT_BYTES)
                                                                 : Blocks();
}

auto Layout::segment_bytes() const -> Option<u64> {
    RSTD_MATCH(*this) {
        RSTD_CASE(Blocks) { return None(); }
        RSTD_CASE(Segments, bytes) { return Some<u64>(bytes); }
    }
}

auto Layout::validate(Geometry geometry) const -> Result<empty, Error> {
    if (geometry.block != MIN_BLOCK && geometry.block != u64(524288) && geometry.block != MAX_BLOCK)
        return Err(format_error("invalid block size"_str));
    if (geometry.count() > u64(1000000))
        return Err(format_error("resource index exceeds memory limit"_str));
    auto bytes = segment_bytes();
    if (bytes.is_some() &&
        ((*bytes != SEGMENT_BYTES && *bytes != u64(33554432)) || *bytes % geometry.block != u64()))
        return Err(format_error("invalid segment geometry"_str));
    return Ok(empty {});
}

auto Index::load_layout(Geometry geometry, Vec<u8> bytes, Option<u64> slots)
    -> Result<Index, Error> {
    if (geometry.block != MIN_BLOCK && geometry.block != u64(524288) && geometry.block != MAX_BLOCK)
        return Err(format_error("invalid block size"_str));
    if (geometry.count() > u64(1000000) ||
        bytes.len() != usize(geometry.count().to_primitive()) * ENTRY_LEN)
        return Err(format_error("invalid index size"_str));
    Index value;
    value.bytes_    = rstd::move(bytes);
    value.geometry_ = geometry;
    for (u64 i {}; i < geometry.count(); ++i) {
        auto entry = value.get(i);
        if (entry.is_some() && slots.is_some() &&
            u64(rstd::get<0>(*entry).to_primitive()) >= *slots)
            return Err(format_error("index references absent slot"_str));
    }
    return Ok(rstd::move(value));
}

auto Index::make(Geometry geometry) -> Index {
    Index value;
    value.geometry_ = geometry;
    value.bytes_.resize(usize(geometry.count().to_primitive()) * ENTRY_LEN, u8());
    return value;
}

auto Index::load(Geometry geometry, Vec<u8> bytes, u64 slots) -> Result<Index, Error> {
    return load_layout(geometry, rstd::move(bytes), Some(slots));
}

auto Index::load_plain(Geometry geometry, Vec<u8> bytes) -> Result<Index, Error> {
    auto value = rstd_try(load_layout(geometry, rstd::move(bytes), None()));
    if (! value.complete()) return Err(format_error("incomplete plain index"_str));
    return Ok(rstd::move(value));
}

auto Index::load_segments(Geometry geometry, Vec<u8> bytes, u64 size) -> Result<Index, Error> {
    auto value = rstd_try(load_layout(geometry, rstd::move(bytes), None()));
    for (u64 i {}; i < geometry.count(); ++i) {
        auto entry = value.get(i);
        if (entry.is_some() &&
            u64(rstd::get<0>(*entry).to_primitive()) != i % (size / geometry.block))
            return Err(format_error("invalid segment slot"_str));
    }
    return Ok(rstd::move(value));
}

auto Index::clone() const -> Index {
    Index value;
    value.bytes_    = bytes_.clone();
    value.geometry_ = geometry_;
    for (const auto& range : dirty_) value.dirty_.push(Range { range.start, range.end });
    return value;
}

auto Index::get(u64 index) const -> Option<tuple<u32, array<u8, 32>>> {
    if (index > u64(usize::MAX.to_primitive())) return None();
    auto start = usize(index.to_primitive()).checked_mul(ENTRY_LEN);
    if (start.is_none()) return None();
    auto end = start->checked_add(ENTRY_LEN);
    if (end.is_none() || *end > bytes_.len()) return None();
    array<u8, 4> slot_bytes {};
    for (usize i {}; i < usize(4); ++i) slot_bytes[i] = bytes_[*start + i];
    auto slot = u32::from_le_bytes(slot_bytes);
    if (slot == u32()) return None();
    array<u8, 32> digest {};
    for (usize i {}; i < usize(32); ++i) digest[i] = bytes_[*start + usize(4) + i];
    return Some(tuple<u32, array<u8, 32>> { slot - u32(1), digest });
}

auto Index::set(u64 index, u32 slot, const Bytes& data) -> Result<empty, Error> {
    auto range = rstd_try(geometry_.range(index));
    if (u64(data.len().to_primitive()) != range.end - range.start)
        return Err(format_error("incomplete block"_str));
    if (slot == u32::MAX) return Err(format_error("physical slot limit reached"_str));
    auto start      = usize(index.to_primitive()) * ENTRY_LEN;
    auto slot_bytes = (slot + u32(1)).to_le_bytes();
    for (usize i {}; i < usize(4); ++i) bytes_[start + i] = slot_bytes[i];
    auto digest = rstd_try(hash(data.as_slice()));
    for (usize i {}; i < usize(32); ++i) bytes_[start + usize(4) + i] = digest[i];
    dirty_.push(Range { u64(start.to_primitive()), u64((start + ENTRY_LEN).to_primitive()) });
    return Ok(empty {});
}

auto Index::complete() const -> bool {
    for (u64 i {}; i < geometry_.count(); ++i)
        if (get(i).is_none()) return false;
    return true;
}

auto Index::dirty() -> Vec<Range> {
    rstd::slice_::sort_by_key(dirty_.as_mut_slice().as_mut_ref(), [](const Range& range) {
        return range.start;
    });
    Vec<Range> ranges;
    for (const auto& range : dirty_) {
        if (! ranges.is_empty()) {
            auto& last = ranges[ranges.len() - usize(1)];
            if (range.start <= last.end) {
                if (range.end > last.end) last.end = range.end;
                continue;
            }
        }
        ranges.push(Range { range.start, range.end });
    }
    return ranges;
}

void Index::remove(u64 index) {
    if (index >= geometry_.count()) rstd::panic { "block out of bounds" };
    auto start = usize(index.to_primitive()) * ENTRY_LEN;
    for (usize i {}; i < ENTRY_LEN; ++i) bytes_[start + i] = u8();
    dirty_.push(Range { u64(start.to_primitive()), u64((start + ENTRY_LEN).to_primitive()) });
}
} // namespace qcm::backend::resource_cache
