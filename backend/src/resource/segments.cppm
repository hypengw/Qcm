module;
#include <rstd/macro.hpp>
export module qcm.backend:resource.segments;
import :resource.storage;
import rstd.log;
using namespace rstd::prelude;
using namespace rstd::literals;
using rstd::tuple;
using rstd::bytes::Bytes;
using rstd::collections::BTreeMap;
using rstd::collections::HashMap;
using rstd::fs::File;
using rstd::sync::Arc;

namespace qcm::backend::resource_cache::segments_detail
{
namespace q = liorm::query;
using cache_db::SegmentRecord;
using cache_db::VersionRecord;
struct SegmentRow {
    String token;
    i64    length;
    i64    deleting;
};
inline constexpr liorm::RecordProjection<SegmentRow, &SegmentRow::token, &SegmentRow::length,
                                         &SegmentRow::deleting>
    segment_projection;
struct RetainedSegmentRow {
    String meta;
    i64    number;
    String token;
    i64    length;
};
inline constexpr liorm::RecordProjection<RetainedSegmentRow, &RetainedSegmentRow::meta,
                                         &RetainedSegmentRow::number, &RetainedSegmentRow::token,
                                         &RetainedSegmentRow::length>
    retained_segment_projection;

inline constexpr liorm::TupleProjection<i64> segment_number_projection;
struct EvictionCandidateRow {
    i64 version_id;
    i64 number;
    i64 deleting;
    i64 obsolete;
};
inline constexpr liorm::RecordProjection<
    EvictionCandidateRow, &EvictionCandidateRow::version_id, &EvictionCandidateRow::number,
    &EvictionCandidateRow::deleting, &EvictionCandidateRow::obsolete>
    eviction_candidate_projection;
template<class Record>
inline constexpr auto last_use            = q::compile([](q::QueryBuilder& b) {
    auto table = b.use(cache_db::schema).table<Record>();
    return b
        .select(b.function(q::Function::Coalesce,
                           b.function(q::Function::Max, table.template col<&Record::last_use>()),
                           b.integer(0)))
        .from(table.source());
});
inline constexpr auto segment             = q::compile([](q::QueryBuilder& b) {
    auto segment = b.use(cache_db::schema).table<SegmentRecord>();
    return segment_projection
        .select(b,
                segment.col<&SegmentRecord::token>(),
                segment.col<&SegmentRecord::length>(),
                segment.col<&SegmentRecord::deleting>())
        .from(segment.source())
        .where((segment.col<&SegmentRecord::version_id>() == b.param<i64>("id"_str)) &&
               (segment.col<&SegmentRecord::number>() == b.param<i64>("number"_str)));
});
inline constexpr auto retained_segments   = q::compile([](q::QueryBuilder& b) {
    auto schema  = b.use(cache_db::schema);
    auto segment = schema.table<SegmentRecord>();
    auto version = schema.table<VersionRecord>();
    return retained_segment_projection
        .select(b,
                version.col<&VersionRecord::meta>(),
                segment.col<&SegmentRecord::number>(),
                segment.col<&SegmentRecord::token>(),
                segment.col<&SegmentRecord::length>())
        .from(segment.source())
        .join(version.source(),
              version.col<&VersionRecord::id>() == segment.col<&SegmentRecord::version_id>());
});
inline constexpr auto touch_segment       = q::compile([](q::QueryBuilder& b) {
    auto segment = b.use(cache_db::schema).table<SegmentRecord>();
    return b.update(segment.source())
        .set(segment.col<&SegmentRecord::last_use>().set(b.param<i64>("touched"_str)))
        .where((segment.col<&SegmentRecord::version_id>() == b.param<i64>("id"_str)) &&
               (segment.col<&SegmentRecord::number>() == b.param<i64>("number"_str)));
});
inline constexpr auto write_segment       = q::compile([](q::QueryBuilder& b) {
    auto segment = b.use(cache_db::schema).table<SegmentRecord>();
    return b
        .insert_into(segment.source(),
                     segment.col<&SegmentRecord::version_id>(),
                     segment.col<&SegmentRecord::number>(),
                     segment.col<&SegmentRecord::token>(),
                     segment.col<&SegmentRecord::length>(),
                     segment.col<&SegmentRecord::last_use>(),
                     segment.col<&SegmentRecord::deleting>())
        .values(b.param<i64>("id"_str),
                b.param<i64>("number"_str),
                b.param<q::TextParameter>("token"_str),
                b.param<i64>("length"_str),
                b.param<i64>("touched"_str),
                b.integer(0))
        .on_conflict_do_update(
            array<q::Expr, 2> { segment.col<&SegmentRecord::version_id>(),
                                segment.col<&SegmentRecord::number>() }
                .as_slice(),
            array<q::Expr, 4> { segment.col<&SegmentRecord::token>().set(
                                    b.excluded(segment.col<&SegmentRecord::token>())),
                                segment.col<&SegmentRecord::length>().set(
                                    b.excluded(segment.col<&SegmentRecord::length>())),
                                segment.col<&SegmentRecord::last_use>().set(
                                    b.excluded(segment.col<&SegmentRecord::last_use>())),
                                segment.col<&SegmentRecord::deleting>().set(b.integer(0)) }
                .as_slice());
});
inline constexpr auto mark_deleting       = q::compile([](q::QueryBuilder& b) {
    auto segment = b.use(cache_db::schema).table<SegmentRecord>();
    return b.update(segment.source())
        .set(segment.col<&SegmentRecord::deleting>().set(b.integer(1)))
        .where((segment.col<&SegmentRecord::version_id>() == b.param<i64>("id"_str)) &&
               (segment.col<&SegmentRecord::number>() == b.param<i64>("number"_str)));
});
inline constexpr auto delete_segment      = q::compile([](q::QueryBuilder& b) {
    auto segment = b.use(cache_db::schema).table<SegmentRecord>();
    return b.delete_from(segment.source())
        .where((segment.col<&SegmentRecord::version_id>() == b.param<i64>("id"_str)) &&
               (segment.col<&SegmentRecord::number>() == b.param<i64>("number"_str)));
});
inline constexpr auto segment_numbers     = q::compile([](q::QueryBuilder& b) {
    auto segment = b.use(cache_db::schema).table<SegmentRecord>();
    return segment_number_projection.select(b, segment.col<&SegmentRecord::number>())
        .from(segment.source())
        .where(segment.col<&SegmentRecord::version_id>() == b.param<i64>("id"_str));
});
inline constexpr auto eviction_candidates = q::compile([](q::QueryBuilder& b) {
    auto schema  = b.use(cache_db::schema);
    auto segment = schema.table<SegmentRecord>();
    auto version = schema.table<VersionRecord>();
    return eviction_candidate_projection
        .select(b,
                segment.col<&SegmentRecord::version_id>(),
                segment.col<&SegmentRecord::number>(),
                segment.col<&SegmentRecord::deleting>(),
                version.col<&VersionRecord::obsolete>())
        .from(segment.source())
        .join(version.source(),
              version.col<&VersionRecord::id>() == segment.col<&SegmentRecord::version_id>())
        .order_by(segment.col<&SegmentRecord::deleting>().desc(),
                  version.col<&VersionRecord::obsolete>().desc(),
                  segment.col<&SegmentRecord::last_use>().asc(),
                  segment.col<&SegmentRecord::rowid>().asc());
});
auto                  hexadecimal(u8 byte) -> Option<u8> {
    if (byte >= u8('0') && byte <= u8('9')) return Some(byte - u8('0'));
    if (byte >= u8('a') && byte <= u8('f')) return Some(byte - u8('a') + u8(10));
    if (byte >= u8('A') && byte <= u8('F')) return Some(byte - u8('A') + u8(10));
    return None();
}
auto ceil_div(u64 value, u64 divisor) -> u64 {
    return value / divisor + (value % divisor == u64() ? u64() : u64(1));
}
auto block_range(const Metadata& meta, u64 number, u64 size) -> DbResult<Range> {
    auto start = number.checked_mul(size);
    if (start.is_none()) return Err(Error::state("invalid segment number"_str));
    if (*start >= meta.geometry.length)
        return Ok(Range { *start / meta.geometry.block, *start / meta.geometry.block });
    return Ok(Range { *start / meta.geometry.block,
                      ceil_div(rstd::cmp::min(start->saturating_add(size), meta.geometry.length),
                               meta.geometry.block) });
}
} // namespace qcm::backend::resource_cache::segments_detail

namespace qcm::backend::resource_cache
{
auto Segment::name(const Metadata& meta) const -> String {
    return rstd::format("{}.{:016x}.{}.seg", meta.generation.as_str(), number, token.as_str());
}
auto Segment::header(const Metadata& meta) const -> DbResult<array<u8, 64>> {
    if (! valid_generation(token.as_str())) return Err(Error::state("invalid segment token"_str));
    array<u8, 64> bytes {};
    auto          magic        = "QCMSEG01"_str.as_bytes();
    auto          length_bytes = length.to_le_bytes();
    auto          number_bytes = number.to_le_bytes();
    auto          block_bytes  = meta.geometry.block.to_le_bytes();
    for (usize i {}; i < usize(8); ++i) {
        bytes[i]             = magic[i];
        bytes[usize(8) + i]  = length_bytes[i];
        bytes[usize(16) + i] = number_bytes[i];
        bytes[usize(24) + i] = block_bytes[i];
    }
    auto digits            = token.as_str().as_bytes();
    auto generation_digest = rstd_try(hash(meta.generation.as_str().as_bytes()));
    for (usize i {}; i < usize(16); ++i) {
        auto high = segments_detail::hexadecimal(digits[i * usize(2)]).unwrap();
        auto low  = segments_detail::hexadecimal(digits[i * usize(2) + usize(1)]).unwrap();
        bytes[usize(32) + i] = high * u8(16) + low;
        bytes[usize(48) + i] = generation_digest[i];
    }
    return Ok(bytes);
}
auto managed_segment_name(ref<str> name) -> bool {
    if (name.len() != usize(86) || name.as_bytes()[usize(32)] != u8('.') ||
        name.as_bytes()[usize(49)] != u8('.') || ! name.ends_with(".seg"_str))
        return false;
    if (! valid_generation(name.get(usize(), usize(32)).unwrap()) ||
        ! valid_generation(name.get(usize(50), usize(82)).unwrap()))
        return false;
    for (auto byte : name.get(usize(33), usize(49)).unwrap().as_bytes())
        if (segments_detail::hexadecimal(byte).is_none()) return false;
    return true;
}
auto Storage::restore_segment_clock() -> DbResult<empty> {
    auto maximum = [&]<class Record>() -> DbResult<i64> {
        auto row = rstd_try(db.query(segments_detail::last_use<Record>));
        return Ok(rstd_try(liorm::sqlito::scalar<i64>(row)));
    };
    auto segments = rstd_try(maximum.template operator()<cache_db::SegmentRecord>());
    auto packs    = rstd_try(maximum.template operator()<cache_db::PackRecord>());
    auto clock    = rstd::cmp::max(segments, packs);
    segment_clock = clock < i64() ? u64() : u64(clock.to_primitive());
    return Ok(empty {});
}
auto Storage::segment_tick() -> u64 {
    auto timestamp = rstd::time::SystemTime::now().as_unix_time();
    u64  current {};
    if (timestamp.seconds >= i64())
        current = rstd::cmp::min(
            u64(timestamp.seconds.to_primitive())
                .saturating_mul(u64(1000000))
                .saturating_add(u64((timestamp.nanoseconds / u32(1000)).to_primitive())),
            u64(9223372036854775807ULL));
    segment_clock = rstd::cmp::max(current, segment_clock.saturating_add(u64(1)));
    return segment_clock;
}
auto Storage::pin_segments(i64 id, Range numbers) -> Window {
    pins.retain([](const auto&, const auto& pin) {
        return pin.strong_count() > usize();
    });
    Window window;
    for (auto number = numbers.start; number < numbers.end; ++number) {
        auto key   = SegmentKey { id, number };
        auto found = pins.get(key);
        auto pin   = found.is_some() ? (*found)->upgrade() : Arc<empty>::make();
        if (! pin) pin = Arc<empty>::make();
        pins.insert(rstd::move(key), pin.downgrade());
        window.pins.push(rstd::move(pin));
    }
    return window;
}
auto Storage::segment(i64 id, u64 number) -> DbResult<Option<Segment>> {
    auto row        = rstd_try(db.query(
        segments_detail::segment, parameter("id"_str, id), parameter("number"_str, number)));
    auto descriptor = rstd_try(liorm::sqlito::one_row(
        row, segments_detail::segment_projection, liorm::sqlito::RowConsumption::First));
    if (descriptor.is_none()) return Ok(None());
    if (! valid_generation(descriptor->token.as_str()) || descriptor->length <= i64())
        return Err(Error::state("invalid segment descriptor"_str));
    return Ok(Some(Segment { number,
                             rstd::move(descriptor->token),
                             u64(descriptor->length.to_primitive()),
                             descriptor->deleting != i64() }));
}
auto Storage::segment_file(const Metadata& meta, const Segment& segment) -> DbResult<File> {
    auto size = meta.layout.segment_bytes();
    if (size.is_none()) return Err(Error::state("not segmented"_str));
    auto start = segment.number.checked_mul(*size);
    if (start.is_none() || *start >= meta.geometry.length ||
        segment.length != rstd::cmp::min(*size, meta.geometry.length - *start) || segment.deleting)
        return Err(Error::state("invalid segment length"_str));
    auto path = root.join(segment.name(meta).as_str());
    rstd_try(regular_file(path.as_path()));
    auto file     = rstd_try(File::options().read(true).write(true).open(path.as_path()));
    auto metadata = rstd_try(file.metadata());
    if (metadata.len() < HEADER_LEN || metadata.len() > HEADER_LEN + segment.length)
        return Err(Error::state("invalid segment file length"_str));
    array<u8, 64> header {};
    rstd_try(file.read_exact_at(header.as_mut_slice(), u64()));
    if (header != rstd_try(segment.header(meta)))
        return Err(Error::state("segment identity mismatch"_str));
    return Ok(rstd::move(file));
}
auto Storage::retain_segments(HashMap<String, ResourceKind>& retained) -> DbResult<empty> {
    auto rows = rstd_try(db.query(segments_detail::retained_segments));
    while (rstd_try(next(rows))) {
        auto reader     = rows.reader();
        auto descriptor = rstd_try(segments_detail::retained_segment_projection.decode(reader));
        auto meta       = decode_metadata(descriptor.meta.as_str());
        if (meta.is_err()) {
            rstd::log::warn("invalid segment metadata: {}", rstd::move(meta).unwrap_err());
            continue;
        }
        if (! valid_generation(meta->generation.as_str()) || meta->layout.segment_bytes().is_none())
            continue;
        auto token  = rstd::move(descriptor.token);
        auto number = descriptor.number;
        if (number < i64() || ! valid_generation(token.as_str())) continue;
        Segment segment { u64(number.to_primitive()),
                          rstd::move(token),
                          u64(descriptor.length.to_primitive()),
                          false };
        retained.insert(segment.name(*meta), meta->kind);
    }
    return Ok(empty {});
}
auto Storage::repair_segments(i64 id) -> DbResult<empty> {
    auto meta = rstd_try(load(id))->meta.clone();
    auto size = meta.layout.segment_bytes();
    if (size.is_none()) return Ok(empty {});
    auto count = segments_detail::ceil_div(meta.geometry.length, *size);
    for (u64 number {}; number < count; ++number) {
        auto range = rstd_try(segments_detail::block_range(meta, number, *size));
        bool present {};
        auto value = versions.get(id);
        if (value.is_none()) return Err(Error::state("version not resident"_str));
        for (auto block = range.start; block < range.end; ++block)
            if ((*value)->index.get(block).is_some()) {
                present = true;
                break;
            }
        if (! present) continue;
        auto descriptor = segment(id, number);
        bool valid {};
        if (descriptor.is_err()) {
            rstd::log::warn("invalid segment descriptor: {}", rstd::move(descriptor).unwrap_err());
        } else if (descriptor->is_some()) {
            auto opened = segment_file(meta, **descriptor);
            valid       = opened.is_ok();
            if (! valid)
                rstd::log::warn("invalid segment discarded: {}", rstd::move(opened).unwrap_err());
        }
        if (! valid) rstd_try(remove_segment(id, number));
    }
    return Ok(empty {});
}
auto Storage::read_segment(i64 id, u64 block) -> DbResult<Option<Bytes>> {
    auto value = rstd_try(load(id));
    auto meta  = value->meta.clone();
    auto entry = value->index.get(block);
    if (entry.is_none()) return Ok(None());
    auto slot       = rstd::get<0>(*entry);
    auto digest     = rstd::get<1>(*entry);
    auto range      = rstd_try(meta.geometry.range(block));
    auto number     = range.start / meta.layout.segment_bytes().unwrap();
    auto descriptor = rstd_try(segment(id, number));
    if (descriptor.is_none()) {
        rstd_try(remove_segment(id, number));
        return Ok(None());
    }
    auto opened = segment_file(meta, *descriptor);
    if (opened.is_err()) {
        rstd::log::warn("segment discarded: {}", rstd::move(opened).unwrap_err());
        rstd_try(remove_segment(id, number));
        return Ok(None());
    }
    auto file  = rstd::move(opened).unwrap();
    auto bytes = Vec<u8>::make();
    bytes.resize(usize((range.end - range.start).to_primitive()), u8());
    auto offset     = HEADER_LEN + u64(slot.to_primitive()) * meta.geometry.block;
    auto read       = file.read_exact_at(bytes.as_mut_slice().as_mut_ref(), offset);
    bool unreadable = read.is_err();
    if (unreadable) rstd::log::warn("segment block read failed: {}", rstd::move(read).unwrap_err());
    if (unreadable || rstd_try(hash(bytes.as_slice())) != digest) {
        auto index = rstd_try(load(id))->index.clone();
        index.remove(block);
        rstd_try(publish_segment_index(id, rstd::move(index), []() -> DbResult<empty> {
            return Ok(empty {});
        }));
        return Ok(None());
    }
    auto touched = segment_tick();
    rstd_try(db.execute(segments_detail::touch_segment,
                        parameter("touched"_str, touched),
                        parameter("id"_str, id),
                        parameter("number"_str, number)));
    return Ok(Some(Bytes::from_vec(rstd::move(bytes))));
}
auto Storage::write_segments(i64 id, const Blocks& blocks) -> DbResult<empty> {
    rstd_try(repair_segments(id));
    auto                                       meta = rstd_try(load(id))->meta.clone();
    auto                                       size = meta.layout.segment_bytes().unwrap();
    BTreeMap<u64, BTreeMap<u64, const Bytes*>> grouped;
    for (const auto& block : blocks) {
        auto        number = rstd::get<0>(block);
        const auto& data   = rstd::get<1>(block);
        auto        range  = rstd_try(meta.geometry.range(number));
        if (u64(data.len().to_primitive()) != range.end - range.start)
            return Err(Error::state("incomplete block"_str));
        if (rstd_try(load(id))->index.get(number).is_some()) continue;
        auto group = range.start / size;
        if (grouped.get(group).is_none())
            grouped.insert(group, BTreeMap<u64, const Bytes*>::make());
        (*grouped.get_mut(group))->insert(number, rstd::addressof(data));
    }
    if (grouped.is_empty()) return Ok(empty {});
    Vec<Window> windows;
    u64         required {};
    for (auto group : grouped.iter()) {
        auto number = *rstd::get<0>(group);
        windows.push(pin_segments(id, Range { number, number + u64(1) }));
        required += u64(8192);
        for (auto block : rstd::get<1>(group)->iter()) {
            auto data = *rstd::get<1>(block);
            required +=
                segments_detail::ceil_div(u64(data->len().to_primitive()), u64(4096)) * u64(4096);
        }
    }
    rstd_try(evict(required));
    auto         index = rstd_try(load(id))->index.clone();
    Vec<Segment> written;
    Vec<String>  created;
    auto         result = [&]() -> DbResult<empty> {
        for (auto group : grouped.iter()) {
            auto            number = *rstd::get<0>(group);
            auto            old    = rstd_try(segment(id, number));
            Option<Segment> descriptor;
            bool            creating = old.is_none() || old->deleting;
            if (! creating) {
                descriptor = Some(rstd::move(*old));
            } else {
                if (old.is_some()) rstd_try(remove_segment(id, number));
                descriptor =
                    Some(Segment { number,
                                   rstd_try(token()),
                                   rstd::cmp::min(size, meta.geometry.length - number * size),
                                   false });
                created.push(descriptor->name(meta));
            }
            auto name = descriptor->name(meta);
            auto open = [&]() -> DbResult<File> {
                if (! creating) return segment_file(meta, *descriptor);
                auto file = rstd_try(File::options().create_new(true).read(true).write(true).open(
                    root.join(name.as_str()).as_path()));
                rstd_try(private_file(file));
                auto header = rstd_try(descriptor->header(meta));
                rstd_try(file.write_all_at(header.as_slice(), u64()));
                return Ok(rstd::move(file));
            };
            auto file    = rstd_try(open());
            auto writing = [&]() -> DbResult<empty> {
                for (auto block : rstd::get<1>(group)->iter()) {
                    auto number = *rstd::get<0>(block);
                    auto data   = *rstd::get<1>(block);
                    auto slot   = u32((number % (size / meta.geometry.block)).to_primitive());
                    auto offset = HEADER_LEN + u64(slot.to_primitive()) * meta.geometry.block;
                    rstd_try(file.write_all_at(data->as_slice(), offset));
                    rstd_try(index.set(number, slot, *data));
                }
                return Ok(rstd_try(file.sync_all()));
            }();
            sizes.insert(rstd::move(name), { rstd_try(allocated(file)), meta.kind });
            rstd_try(rstd::move(writing));
            written.push(rstd::move(*descriptor));
        }
        rstd_try(sync_directory(root.as_path()));
        auto touched = segment_tick();
        return publish_segment_index(id, rstd::move(index), [&]() -> DbResult<empty> {
            for (const auto& segment : written)
                rstd_try(db.execute(segments_detail::write_segment,
                                    parameter("id"_str, id),
                                    parameter("number"_str, segment.number),
                                    parameter("token"_str, segment.token.as_str()),
                                    parameter("length"_str, segment.length),
                                    parameter("touched"_str, touched)));
            return Ok(empty {});
        });
    }();
    if (result.is_err())
        for (const auto& name : created) (void)remove_managed(name.as_str());
    return result;
}
auto Storage::remove_segment(i64 id, u64 number) -> DbResult<empty> {
    auto value = rstd_try(load(id));
    auto meta  = value->meta.clone();
    auto size  = meta.layout.segment_bytes();
    if (size.is_none()) return Err(Error::state("not segmented"_str));
    auto index = value->index.clone();
    auto range = rstd_try(segments_detail::block_range(meta, number, *size));
    for (auto block = range.start; block < range.end; ++block)
        if (index.get(block).is_some()) index.remove(block);
    rstd_try(publish_segment_index(id, rstd::move(index), [&]() -> DbResult<empty> {
        return Ok(rstd_try(db.execute(segments_detail::mark_deleting,
                                      parameter("id"_str, id),
                                      parameter("number"_str, number))));
    }));
    auto descriptor = segment(id, number);
    if (descriptor.is_ok() && descriptor->is_some())
        rstd_try(remove_managed((**descriptor).name(meta).as_str()));
    return Ok(rstd_try(db.execute(segments_detail::delete_segment,
                                  parameter("id"_str, id),
                                  parameter("number"_str, number))));
}
auto Storage::delete_all_segments(i64 id) -> DbResult<empty> {
    Vec<u64> numbers;
    {
        auto rows = rstd_try(db.query(segments_detail::segment_numbers, parameter("id"_str, id)));
        auto values =
            rstd_try(liorm::sqlito::all_rows(rows, segments_detail::segment_number_projection));
        for (const auto& value : values) numbers.push(u64(rstd::get<0>(value).to_primitive()));
    }
    for (auto number : numbers) rstd_try(remove_segment(id, number));
    return Ok(empty {});
}
auto Storage::evict_segments(u64 required) -> DbResult<empty> {
    Vec<segments_detail::EvictionCandidateRow> candidates;
    {
        auto rows = rstd_try(db.query(segments_detail::eviction_candidates));
        candidates =
            rstd_try(liorm::sqlito::all_rows(rows, segments_detail::eviction_candidate_projection));
    }
    for (const auto& candidate : candidates) {
        auto used = rstd_try(usage());
        if (candidate.deleting == i64() && candidate.obsolete == i64() &&
            used.image.saturating_add(used.audio)
                    .saturating_add(used.metadata)
                    .saturating_add(required) <= capacity)
            continue;
        auto id     = candidate.version_id;
        auto number = u64(candidate.number.to_primitive());
        auto pin    = pins.get(SegmentKey { id, number });
        if (pin.is_some() && (*pin)->upgrade()) continue;
        if (auto cleanup = remove_segment(id, number); cleanup.is_err())
            rstd::log::warn("segment deletion deferred: {}", rstd::move(cleanup).unwrap_err());
    }
    return Ok(empty {});
}
} // namespace qcm::backend::resource_cache
