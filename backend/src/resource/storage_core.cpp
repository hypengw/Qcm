module;
#include <rstd/macro.hpp>
#ifndef _WIN32
#    include <fcntl.h>
#    include <sys/stat.h>
#endif
module qcm.backend;
import :resource.storage;
import :resource.schema;
import :resource.startup;
import liuuid;
import rstd.log;
using namespace rstd::prelude;
using namespace rstd::literals;
using rstd::tuple;
using rstd::bytes::Bytes;
using rstd::collections::HashMap;
using rstd::fs::File;
using rstd::path::Path;
using rstd::path::PathBuf;
using rstd::sync::Arc;
using rstd::sync::Weak;
using rstd::sync::atomic::Ordering;

namespace qcm::backend::resource_cache::core_queries
{
namespace q = liorm::query;
using namespace cache_db;
struct RecoveryRow {
    i64    id;
    String meta;
    i64    ready;
};
inline constexpr liorm::RecordProjection<RecoveryRow, &RecoveryRow::id, &RecoveryRow::meta,
                                         &RecoveryRow::ready>
    recovery_projection;
struct GenerationRow {
    String generation;
    i64    kind;
};
inline constexpr liorm::RecordProjection<GenerationRow, &GenerationRow::generation,
                                         &GenerationRow::kind>
    generation_projection;
struct VersionData {
    String  meta;
    Vec<u8> blocks;
};
inline constexpr liorm::RecordProjection<VersionData, &VersionData::meta, &VersionData::blocks>
    version_data_projection;
struct EvictionRow {
    i64 id;
    i64 obsolete;
};
inline constexpr liorm::RecordProjection<EvictionRow, &EvictionRow::id, &EvictionRow::obsolete>
    eviction_projection;

inline constexpr auto recovery           = q::compile([](q::QueryBuilder& b) {
    auto tables = b.use(schema);
    auto v      = tables.table<VersionRecord>();
    auto f      = tables.table<CompleteFileRecord>();
    return recovery_projection
        .select(
            b,
            v.col<&VersionRecord::id>(),
            v.col<&VersionRecord::meta>(),
            b.function(q::Function::Coalesce, f.col<&CompleteFileRecord::ready>(), b.integer(0)))
        .from(v.source())
        .left_join(f.source(),
                   f.col<&CompleteFileRecord::version_id>() == v.col<&VersionRecord::id>());
});
inline constexpr auto leased_generations = q::compile([](q::QueryBuilder& b) {
    auto tables = b.use(schema);
    auto v      = tables.table<VersionRecord>();
    auto l      = tables.table<FileLeaseRecord>();
    return generation_projection
        .select(b, v.col<&VersionRecord::generation>(), v.col<&VersionRecord::kind>())
        .distinct()
        .from(v.source())
        .join(l.source(), l.col<&FileLeaseRecord::version_id>() == v.col<&VersionRecord::id>());
});
inline constexpr auto version_data       = q::compile([](q::QueryBuilder& b) {
    auto v = b.use(schema).table<VersionRecord>();
    return version_data_projection
        .select(b, v.col<&VersionRecord::meta>(), v.col<&VersionRecord::blocks>())
        .from(v.source())
        .where(v.col<&VersionRecord::id>() == b.param<i64>("id"_str));
});
template<auto Member>
inline constexpr auto version_field       = q::compile([](q::QueryBuilder& b) {
    auto v = b.use(schema).table<VersionRecord>();
    return b.select(v.col<Member>())
        .from(v.source())
        .where(v.col<&VersionRecord::id>() == b.param<i64>("id"_str));
});
inline constexpr auto latest_version      = q::compile([](q::QueryBuilder& b) {
    auto v = b.use(schema).table<VersionRecord>();
    return b.select(v.col<&VersionRecord::id>())
        .from(v.source())
        .where((v.col<&VersionRecord::resource_key>() == b.param<q::TextParameter>("key"_str)) &&
               (v.col<&VersionRecord::obsolete>() == b.integer(0)))
        .order_by(v.col<&VersionRecord::id>().desc())
        .limit(b.integer(1));
});
inline constexpr auto touch               = q::compile([](q::QueryBuilder& b) {
    auto v = b.use(schema).table<VersionRecord>();
    return b.update(v.source())
        .set(v.col<&VersionRecord::last_use>().set(b.param<i64>("time"_str)))
        .where(v.col<&VersionRecord::id>() == b.param<i64>("id"_str));
});
inline constexpr auto replace_versions    = q::compile([](q::QueryBuilder& b) {
    auto v = b.use(schema).table<VersionRecord>();
    return b.update(v.source())
        .set(v.col<&VersionRecord::obsolete>().set(b.integer(1)))
        .where((v.col<&VersionRecord::resource_key>() == b.param<q::TextParameter>("key"_str)) &&
               (v.col<&VersionRecord::obsolete>() == b.integer(0)));
});
inline constexpr auto insert_version      = q::compile([](q::QueryBuilder& b) {
    auto v = b.use(schema).table<VersionRecord>();
    return b
        .insert_into(v.source(),
                     v.col<&VersionRecord::resource_key>(),
                     v.col<&VersionRecord::provider>(),
                     v.col<&VersionRecord::kind>(),
                     v.col<&VersionRecord::generation>(),
                     v.col<&VersionRecord::meta>(),
                     v.col<&VersionRecord::blocks>(),
                     v.col<&VersionRecord::last_use>())
        .values(b.param<q::TextParameter>("key"_str),
                b.param<i64>("provider"_str),
                b.param<i64>("kind"_str),
                b.param<q::TextParameter>("generation"_str),
                b.param<q::TextParameter>("meta"_str),
                b.param<q::BlobParameter>("blocks"_str),
                b.param<i64>("time"_str))
        .returning(v.col<&VersionRecord::id>());
});
inline constexpr auto update_meta         = q::compile([](q::QueryBuilder& b) {
    auto v = b.use(schema).table<VersionRecord>();
    return b.update(v.source())
        .set(v.col<&VersionRecord::meta>().set(b.param<q::TextParameter>("meta"_str)))
        .where(v.col<&VersionRecord::id>() == b.param<i64>("id"_str));
});
inline constexpr auto invalidate_version  = q::compile([](q::QueryBuilder& b) {
    auto v = b.use(schema).table<VersionRecord>();
    return b.update(v.source())
        .set(v.col<&VersionRecord::obsolete>().set(b.integer(2)))
        .where(v.col<&VersionRecord::id>() == b.param<i64>("id"_str));
});
inline constexpr auto invalidate_provider = q::compile([](q::QueryBuilder& b) {
    auto v = b.use(schema).table<VersionRecord>();
    return b.update(v.source())
        .set(v.col<&VersionRecord::obsolete>().set(b.integer(2)))
        .where(v.col<&VersionRecord::provider>() == b.param<i64>("provider"_str));
});
template<class Record, auto Member>
inline constexpr auto delete_version_rows = q::compile([](q::QueryBuilder& b) {
    auto t = b.use(schema).table<Record>();
    return b.delete_from(t.source()).where(t.template col<Member>() == b.param<i64>("id"_str));
});
template<bool Over>
inline constexpr auto eviction = q::compile([](q::QueryBuilder& b) {
    auto v      = b.use(schema).table<VersionRecord>();
    auto result = eviction_projection
                      .select(b, v.col<&VersionRecord::id>(), v.col<&VersionRecord::obsolete>())
                      .from(v.source());
    if constexpr (Over)
        return result.order_by(v.col<&VersionRecord::obsolete>().desc(),
                               v.col<&VersionRecord::last_use>().asc());
    else
        return result.where(v.col<&VersionRecord::obsolete>() != b.integer(0));
});
} // namespace qcm::backend::resource_cache::core_queries

namespace qcm::backend::resource_cache
{
auto valid_generation(ref<str> value) -> bool {
    return value.len() == usize(32) && liuuid::Uuid::parse_str(value).is_ok();
}
auto token() -> DbResult<String> { return Ok(rstd_try(liuuid::Uuid::new_v4()).simple()); }
auto sync_directory(ref<Path> path) -> DbResult<empty> {
#ifndef _WIN32
    auto file = rstd_try(rstd::fs::File::open(path));
    rstd_try(file.sync_all());
#else
    (void)path;
#endif
    return Ok(empty {});
}
auto allocated(const File& file) -> DbResult<u64> {
#ifndef _WIN32
    struct stat status {};
    if (::fstat(file.as_raw_fd(), &status) != 0)
        return Err(Error::io(rstd::io::Error::last_os_error()));
    return Ok(u64(static_cast<unsigned long long>(status.st_blocks)) * u64(512));
#else
    auto meta = rstd_try(file.metadata());
    return Ok(meta.len());
#endif
}
auto allocated(ref<Path> path) -> DbResult<u64> {
    auto file = rstd_try(rstd::fs::File::open(path));
    return allocated(file);
}
auto open_owner(ref<Path> root) -> DbResult<File> {
    auto lock_path = PathBuf::from(root).join("owner.lock"_str);
    auto options   = File::options();
    options.read(true).write(true).create(true).mode(u32(0600));
#ifndef _WIN32
    options.custom_flags(i32(O_NOFOLLOW));
#endif
    auto owner      = rstd_try(options.open(lock_path.as_path()));
    auto owner_meta = rstd_try(owner.metadata());
    if (! owner_meta.is_file()) return Err(Error::state("cache owner is not a regular file"_str));
    rstd_try(owner.try_lock());
    rstd_try(private_file(owner));
    return Ok(rstd::move(owner));
}
Storage::Storage(PathBuf path, File lock): root(rstd::move(path)), owner(rstd::move(lock)) {}

auto Storage::open(sqlito::Connection& connection, PathBuf root, File owner, StartupTrace& trace)
    -> DbResult<Storage> {
    Storage store(rstd::move(root), rstd::move(owner));
    store.db.attach(connection);
    rstd_try(trace.measure(StartupStage::Migrate, [&] {
        return store.db.migrate();
    }));
    if (auto capacity = rstd::env::var("QCM_RESOURCE_CACHE_BYTES"_str)) {
        auto parsed = rstd::from_str<u64>(capacity->as_str());
        if (parsed.is_ok()) store.capacity = *parsed;
    }
    store.clock = now();
    rstd_try(trace.measure(StartupStage::SegmentClock, [&] {
        return store.restore_segment_clock();
    }));
    rstd_try(trace.measure(StartupStage::Leases, [&] {
        return store.restore_leases();
    }));
    rstd_try(trace.measure(StartupStage::Recovery, [&] {
        return store.recover(trace);
    }));
    return Ok(rstd::move(store));
}
auto Storage::path(ref<str> generation) const -> PathBuf {
    return root.join(rstd::format("{}.data", generation).as_str());
}
auto Storage::recover(StartupTrace& trace) -> DbResult<empty> {
    HashMap<String, ResourceKind> retained;
    Vec<i64>                      invalid;
    usize version_count {}, lease_count {}, directory_count {}, retained_files {}, orphan_files {};
    {
        auto scope = trace.span(StartupStage::Versions);
        auto rows  = rstd_try(db.query(core_queries::recovery));
        while (rstd_try(next(rows))) {
            auto reader = rows.reader();
            auto row    = rstd_try(core_queries::recovery_projection.decode(reader));
            ++version_count;
            auto meta = rstd_try(decode_metadata(row.meta.as_str()));
            if (! valid_generation(meta.generation.as_str())) {
                invalid.push(i64(row.id));
            } else if (meta.layout.segment_bytes().is_none()) {
                auto suffix = row.ready == i64(1) ? "ok"_str : "data"_str;
                retained.insert(rstd::format("{}.{}", meta.generation, suffix), meta.kind);
            }
        }
    }
    {
        auto scope = trace.span(StartupStage::RetainedLeases);
        auto rows  = rstd_try(db.query(core_queries::leased_generations));
        while (rstd_try(next(rows))) {
            auto reader = rows.reader();
            auto row    = rstd_try(core_queries::generation_projection.decode(reader));
            ++lease_count;
            if (valid_generation(row.generation.as_str()))
                retained.insert(rstd::format("{}.ok", row.generation),
                                rstd_try(decode_kind(row.kind)));
        }
    }
    {
        auto scope = trace.span(StartupStage::InvalidVersions);
        for (auto id : invalid) {
            rstd_try(db.execute(core_queries::invalidate_version, parameter("id"_str, id)));
            if (auto cleanup = erase_version(id); cleanup.is_err())
                rstd::log::warn("invalid cache cleanup deferred: {}",
                                rstd::move(cleanup).unwrap_err());
        }
    }
    auto retained_before = retained.len();
    rstd_try(trace.measure(StartupStage::Segments, [&] {
        return retain_segments(retained);
    }));
    auto segment_files = retained.len() - retained_before;
    retained_before    = retained.len();
    rstd_try(trace.measure(StartupStage::Packs, [&] {
        return retain_packs(retained);
    }));
    auto pack_files = retained.len() - retained_before;
    {
        auto scope     = trace.span(StartupStage::Directory);
        auto directory = rstd_try(rstd::fs::read_dir(root.as_path()));
        while (auto item = directory.next()) {
            ++directory_count;
            auto entry    = rstd_try(rstd::move(*item));
            auto filename = entry.file_name();
            auto name     = filename.as_os_str().to_str();
            if (name.is_none()) continue;
            auto kind = retained.get(String::make(*name));
            if (kind.is_some()) {
                ++retained_files;
                sizes.insert(String::make(*name),
                             { rstd_try(allocated(entry.path().as_path())), **kind });
            } else if (managed_file_name(*name) || managed_segment_name(*name) ||
                       managed_pack_name(*name)) {
                ++orphan_files;
                sizes.insert(String::make(*name),
                             { rstd_try(allocated(entry.path().as_path())), ResourceKind::Image });
                if (auto cleanup = remove_managed(*name); cleanup.is_err())
                    rstd::log::warn("cache cleanup deferred: {}", rstd::move(cleanup).unwrap_err());
            }
        }
    }
    if (auto cleanup = trace.measure(StartupStage::Eviction,
                                     [&] {
                                         return evict(u64());
                                     });
        cleanup.is_err())
        rstd::log::warn("cache recovery over budget: {}", rstd::move(cleanup).unwrap_err());
    rstd::log::debug(rstd::log::Target { "qcm.startup"_str },
                     "resource recovery versions={} leased={} invalid={} segment_files={} "
                     "pack_files={} entries={} retained_files={} orphan_files={}",
                     version_count,
                     lease_count,
                     invalid.len(),
                     segment_files,
                     pack_files,
                     directory_count,
                     retained_files,
                     orphan_files);
    return Ok(empty {});
}
auto Storage::load(i64 id) -> DbResult<Resident*> {
    if (auto found = versions.get_mut(id)) return Ok(rstd::addressof(**found));
    Option<Metadata> metadata;
    Vec<u8>          bytes;
    {
        auto row    = rstd_try(db.query(core_queries::version_data, parameter("id"_str, id)));
        auto stored = rstd_try(liorm::sqlito::one_row(
            row, core_queries::version_data_projection, liorm::sqlito::RowConsumption::First));
        if (stored.is_none()) return Err(Error::state("cache version removed"_str));
        metadata = Some(rstd_try(decode_metadata(stored->meta.as_str())));
        bytes    = rstd::move(stored->blocks);
    }
    auto meta = rstd::move(*metadata);
    if (! valid_generation(meta.generation.as_str()))
        return Err(Error::state("invalid generation"_str));
    rstd_try(meta.layout.validate(meta.geometry));
    rstd_try(reserve_index(bytes.len()));
    if (auto size = meta.layout.segment_bytes()) {
        auto index = rstd_try(Index::load_segments(meta.geometry, rstd::move(bytes), *size));
        versions.insert(
            id,
            Resident { rstd::move(meta), rstd::move(index), None(), Weak<Lease>::make(), false });
        rstd_try(repair_segments(id));
        return Ok(rstd::addressof(**versions.get_mut(id)));
    }
    auto length = rstd_try(plain_length(id));
    bool plain  = length.is_some();
    auto filename =
        plain ? file_path(meta.generation.as_str(), "ok"_str) : path(meta.generation.as_str());
    rstd_try(regular_file(filename.as_path()));
    auto options = File::options();
    options.read(true).write(! plain);
    auto          file = rstd_try(options.open(filename.as_path()));
    auto          info = rstd_try(file.metadata());
    auto          size = info.len();
    Option<Index> index;
    if (plain) {
        if (*length != meta.geometry.length || size != meta.geometry.length)
            return Err(Error::state("invalid plain length"_str));
        index = Some(rstd_try(Index::load_plain(meta.geometry, rstd::move(bytes))));
    } else {
        if (size < HEADER_LEN) return Err(Error::state("incomplete data header"_str));
        array<u8, 64> header {};
        rstd_try(file.read_exact_at(header.as_mut_slice(), u64()));
        if (header != rstd_try(meta.geometry.header(meta.generation.as_str())))
            return Err(Error::state("data identity mismatch"_str));
        auto slots = (size - HEADER_LEN) / meta.geometry.block;
        index      = Some(rstd_try(Index::load(meta.geometry, rstd::move(bytes), slots)));
        rstd_try(file.set_len(HEADER_LEN + slots * meta.geometry.block));
    }
    sizes.insert(rstd::format("{}.{}", meta.generation, plain ? "ok"_str : "data"_str),
                 { rstd_try(allocated(file)), meta.kind });
    versions.insert(id,
                    Resident { rstd::move(meta),
                               rstd::move(*index),
                               Some(rstd::move(file)),
                               Weak<Lease>::make(),
                               plain });
    return Ok(rstd::addressof(**versions.get_mut(id)));
}
auto Storage::version(i64 id) -> DbResult<Version> {
    auto value = rstd_try(load(id));
    auto lease = value->lease.upgrade();
    if (! lease) lease = Arc<Lease>::make();
    value->lease = lease.downgrade();
    return Ok(Version { id, value->meta.clone(), rstd::move(lease) });
}
auto Storage::lookup(ref<str> key) -> DbResult<Option<Version>> {
    Option<i64> id;
    {
        auto row = rstd_try(db.query(core_queries::latest_version, parameter("key"_str, key)));
        id       = rstd_try(liorm::sqlito::optional_scalar<i64>(row));
    }
    if (id.is_some()) {
        auto value = version(*id);
        if (value.is_ok()) {
            rstd_try(db.execute(
                core_queries::touch, parameter("time"_str, now()), parameter("id"_str, *id)));
            return Ok(Some(rstd::move(*value)));
        }
        rstd::log::warn("discarding invalid cache version: {}", rstd::move(value).unwrap_err());
        rstd_try(erase_version(*id));
    }
    return Ok(None());
}
auto Storage::create(Metadata meta) -> DbResult<Version> {
    meta.layout = Layout::for_resource(meta.kind, meta.geometry.length);
    return create_layout(rstd::move(meta));
}
auto Storage::create_layout(Metadata meta) -> DbResult<Version> {
    if (! valid_generation(meta.generation.as_str()))
        return Err(Error::state("invalid generation"_str));
    rstd_try(meta.layout.validate(meta.geometry));
    rstd_try(reserve_index(usize(meta.geometry.count().to_primitive()) * ENTRY_LEN));
    Option<File> file;
    if (meta.layout.segment_bytes().is_none()) {
        rstd_try(evict(u64(4096)));
        auto options = File::options();
        options.read(true).write(true).create_new(true).mode(u32(0600));
        file = Some(rstd_try(options.open(path(meta.generation.as_str()).as_path())));
        rstd_try(private_file(*file));
        auto header = rstd_try(meta.geometry.header(meta.generation.as_str()));
        rstd_try(file->write_all_at(header.as_slice(), u64()));
        rstd_try(file->sync_all());
        rstd_try(sync_directory(root.as_path()));
        sizes.insert(rstd::format("{}.data", meta.generation),
                     { rstd_try(allocated(*file)), meta.kind });
    }
    auto index    = Index::make(meta.geometry);
    auto encoded  = rstd_try(encode_metadata(meta));
    auto inserted = db.atomic([&]() -> DbResult<i64> {
        rstd_try(
            db.execute(core_queries::replace_versions, parameter("key"_str, meta.key.as_str())));
        auto row = rstd_try(db.query(core_queries::insert_version,
                                     parameter("key"_str, meta.key.as_str()),
                                     parameter("provider"_str, meta.provider),
                                     parameter("kind"_str, rstd_try(encode_kind(meta.kind))),
                                     parameter("generation"_str, meta.generation.as_str()),
                                     parameter("meta"_str, encoded.as_str()),
                                     parameter("blocks"_str, index.bytes()),
                                     parameter("time"_str, now())));
        return Ok(rstd_try(liorm::sqlito::scalar<i64>(row)));
    });
    if (inserted.is_err()) {
        file = None();
        (void)remove_managed(rstd::format("{}.data", meta.generation).as_str());
        return Err(rstd::move(inserted).unwrap_err());
    }
    auto id = *inserted;
    versions.insert(
        id,
        Resident {
            rstd::move(meta), rstd::move(index), rstd::move(file), Weak<Lease>::make(), false });
    return version(id);
}
auto Storage::read(i64 id, u64 block) -> DbResult<Option<Bytes>> {
    rstd_try(check_active(id));
    auto value = rstd_try(load(id));
    if (value->meta.layout.segment_bytes().is_some()) return read_segment(id, block);
    auto entry = value->index.get(block);
    if (entry.is_none()) return Ok(None());
    auto range = rstd_try(value->meta.geometry.range(block));
    auto bytes = Vec<u8>::make();
    bytes.resize(usize((range.end - range.start).to_primitive()), u8());
    auto offset =
        value->plain ? range.start : rstd_try(value->meta.geometry.offset(rstd::get<0>(*entry)));
    auto read       = value->file->read_exact_at(bytes.as_mut_slice().as_mut_ref(), offset);
    bool unreadable = read.is_err();
    if (unreadable) rstd::log::warn("cached block read failed: {}", rstd::move(read).unwrap_err());
    if (unreadable || rstd_try(hash(bytes.as_slice())) != rstd::get<1>(*entry)) {
        if (value->plain) {
            rstd_try(retire_corrupt(id));
            return Err(Error::plain_corrupt());
        }
        value->index.remove(block);
        rstd_try(commit(id));
        return Ok(None());
    }
    return Ok(Some(Bytes::from_vec(rstd::move(bytes))));
}
auto Storage::write(i64 id, const Blocks& blocks) -> DbResult<CommitOutcome> {
    rstd_try(check_active(id));
    auto value        = rstd_try(load(id));
    bool was_complete = value->index.is_committed() && value->index.complete();
    if (value->meta.layout.segment_bytes().is_some()) {
        rstd_try(write_segments(id, blocks));
        auto complete = rstd_try(load(id))->index.complete();
        return Ok(CommitOutcome { complete, complete && ! was_complete });
    }
    auto result = write_data(id, blocks);
    value       = rstd_try(load(id));
    sizes.insert(
        rstd::format("{}.{}", value->meta.generation, value->plain ? "ok"_str : "data"_str),
        { rstd_try(allocated(*value->file)), value->meta.kind });
    if (result.is_err()) return Err(rstd::move(result).unwrap_err());
    bool complete = value->index.is_committed() && value->index.complete();
    return Ok(CommitOutcome { complete, complete && ! was_complete });
}
auto Storage::write_data(i64 id, const Blocks& blocks) -> DbResult<empty> {
    rstd_try(check_active(id));
    auto value = rstd_try(load(id));
    if (value->plain) {
        for (const auto& block : blocks) {
            auto entry = value->index.get(rstd::get<0>(block));
            if (entry.is_none() ||
                rstd_try(hash(rstd::get<1>(block).as_slice())) != rstd::get<1>(*entry))
                return Err(Error::state("attempted plain overwrite"_str));
        }
        return Ok(empty {});
    }
    u64 required {};
    for (const auto& block : blocks)
        if (value->index.get(rstd::get<0>(block)).is_none()) required += value->meta.geometry.block;
    rstd_try(evict(required));
    value = rstd_try(load(id));
    Vec<tuple<u64, u32, const Bytes*>> appended;
    for (const auto& block : blocks) {
        auto        number = rstd::get<0>(block);
        const auto& data   = rstd::get<1>(block);
        if (value->index.get(number).is_some()) continue;
        auto range = rstd_try(value->meta.geometry.range(number));
        if (u64(data.len().to_primitive()) != range.end - range.start)
            return Err(Error::state("incomplete block"_str));
        auto metadata = rstd_try(value->file->metadata());
        auto length   = metadata.len() - HEADER_LEN;
        auto slots    = length / value->meta.geometry.block +
                        (length % value->meta.geometry.block != u64() ? u64(1) : u64());
        if (slots >= u64(4294967295ULL)) return Err(Error::state("slot limit"_str));
        auto slot   = u32(slots.to_primitive());
        auto offset = rstd_try(value->meta.geometry.offset(slot));
        rstd_try(value->file->write_all_at(data.as_slice(), offset));
        auto padding = value->meta.geometry.block - u64(data.len().to_primitive());
        if (padding != u64()) {
            auto zeros = Vec<u8>::make();
            zeros.resize(usize(padding.to_primitive()), u8());
            rstd_try(value->file->write_all_at(zeros.as_slice(),
                                               offset + u64(data.len().to_primitive())));
        }
        appended.push({ number, slot, rstd::addressof(data) });
    }
    rstd_try(value->file->sync_data());
    for (const auto& block : appended)
        rstd_try(value->index.set(rstd::get<0>(block), rstd::get<1>(block), *rstd::get<2>(block)));
    return commit(id);
}
auto Storage::patch_index(i64 id, Index& index) -> DbResult<empty> {
    auto bytes = index.bytes();
    auto blob =
        rstd_try(db.open_blob("versions"_str, "blocks"_str, id, sqlito::BlobMode::ReadWrite));
    if (rstd_try(blob.len()) != bytes.len())
        return Err(Error::state("cache index BLOB length changed"_str));
    for (const auto& range : index.dirty()) {
        if (range.start > range.end || range.end > u64(bytes.len().to_primitive()))
            return Err(Error::state("cache index dirty range out of bounds"_str));
        auto start = usize(range.start.to_primitive()), end = usize(range.end.to_primitive());
        auto patch =
            slice<u8>::from_raw_parts(bytes.as_raw_ptr() + start.to_primitive(), end - start);
        rstd_try(blob.write_at(patch, start));
    }
    return Ok(rstd_try(blob.close()));
}
auto Storage::commit(i64 id) -> DbResult<empty> {
    auto found = versions.get_mut(id);
    if (found.is_none()) return Err(Error::state("version not resident"_str));
    auto& index = (*found)->index;
    if (index.is_committed()) return Ok(empty {});
    rstd_try(db.atomic([&] {
        return patch_index(id, index);
    }));
    index.committed();
    return Ok(empty {});
}
auto Storage::update(i64 id, Metadata meta) -> DbResult<empty> {
    auto encoded = rstd_try(encode_metadata(meta));
    rstd_try(db.execute(core_queries::update_meta,
                        parameter("meta"_str, encoded.as_str()),
                        parameter("id"_str, id)));
    rstd_try(load(id))->meta = rstd::move(meta);
    return Ok(empty {});
}
auto Storage::check_active(i64 id) -> DbResult<empty> {
    auto row = rstd_try(db.query(core_queries::version_field<&cache_db::VersionRecord::obsolete>,
                                 parameter("id"_str, id)));
    auto obsolete = rstd_try(liorm::sqlito::optional_scalar<i64>(row));
    if (obsolete.is_none() || *obsolete == i64(2)) return Err(Error::invalidated());
    return Ok(empty {});
}
auto Storage::invalidate(i64 provider) -> DbResult<empty> {
    rstd_try(invalidate_packs(provider));
    for (auto entry : versions.iter_mut()) {
        auto& value = *rstd::get<1>(entry);
        if (value.meta.provider == provider) {
            auto lease = value.lease.upgrade();
            if (lease) lease->valid.store(false, Ordering::Release);
        }
    }
    rstd_try(db.execute(core_queries::invalidate_provider, parameter("provider"_str, provider)));
    return evict(u64());
}
auto Storage::reserve_index(usize required) -> DbResult<empty> {
    for (;;) {
        usize       used {};
        Option<i64> old;
        for (auto entry : versions.iter()) {
            const auto& value = *rstd::get<1>(entry);
            used += value.index.bytes().len();
            if (value.lease.strong_count() == usize()) old = Some(i64(*rstd::get<0>(entry)));
        }
        if (used + required <= usize(64 * 1024 * 1024) && versions.len() < MAX_RESIDENT)
            return Ok(empty {});
        if (old.is_none()) return Err(Error::state("resource index budget reached"_str));
        rstd_try(commit(*old));
        versions.remove(*old);
    }
}
auto Storage::erase_version(i64 id) -> DbResult<empty> {
    for (auto pin : pins.iter())
        if (rstd::get<0>(pin)->version == id && rstd::get<1>(pin)->strong_count() > usize())
            return Ok(empty {});
    if (auto value = versions.get(id))
        if ((*value)->lease.strong_count() > usize()) return Ok(empty {});
    if (rstd_try(externally_pinned(id))) return Ok(empty {});
    Option<String> generation;
    {
        auto row =
            rstd_try(db.query(core_queries::version_field<&cache_db::VersionRecord::generation>,
                              parameter("id"_str, id)));
        generation = rstd_try(liorm::sqlito::optional_scalar<String>(row));
    }
    rstd_try(db.execute(core_queries::invalidate_version, parameter("id"_str, id)));
    rstd_try(delete_all_segments(id));
    versions.remove(id);
    if (generation.is_some() && valid_generation(generation->as_str()))
        for (auto suffix : array<ref<str>, 3> { "data"_str, "ok.tmp"_str, "ok"_str })
            rstd_try(remove_managed(rstd::format("{}.{}", *generation, suffix).as_str()));
    return db.atomic([&]() -> DbResult<empty> {
        rstd_try(db.execute(core_queries::delete_version_rows<cache_db::SegmentRecord,
                                                              &cache_db::SegmentRecord::version_id>,
                            parameter("id"_str, id)));
        rstd_try(
            db.execute(core_queries::delete_version_rows<cache_db::CompleteFileRecord,
                                                         &cache_db::CompleteFileRecord::version_id>,
                       parameter("id"_str, id)));
        rstd_try(
            db.execute(core_queries::delete_version_rows<cache_db::FileLeaseRecord,
                                                         &cache_db::FileLeaseRecord::version_id>,
                       parameter("id"_str, id)));
        return db.execute(core_queries::delete_version_rows<cache_db::VersionRecord,
                                                            &cache_db::VersionRecord::id>,
                          parameter("id"_str, id));
    });
}
auto Storage::usage() -> DbResult<Usage> {
    Usage result;
    for (auto entry : sizes.iter()) {
        const auto& size = *rstd::get<1>(entry);
        if (rstd::get<1>(size) == ResourceKind::Audio)
            result.audio += rstd::get<0>(size);
        else
            result.image += rstd::get<0>(size);
    }
    for (auto suffix : array<ref<str>, 3> { ""_str, "-wal"_str, "-shm"_str }) {
        auto path = root.join(rstd::format("meta.sqlite{}", suffix).as_str());
        auto meta = rstd::fs::metadata(path.as_path());
        if (meta.is_ok())
            result.metadata += rstd_try(allocated(path.as_path()));
        else {
            auto cause = rstd::move(meta).unwrap_err();
            if (cause.kind().code != rstd::io::ErrorKind::NotFound)
                return Err(Error::io(rstd::move(cause)));
        }
    }
    return Ok(result);
}
auto Storage::evict(u64 required) -> DbResult<empty> {
    if (building.is_some()) required = required.saturating_add(building->remaining);
    auto current = rstd_try(usage());
    bool over    = current.image.saturating_add(current.audio)
                       .saturating_add(current.metadata)
                       .saturating_add(required) > capacity;
    if (over && ! garbage.is_empty()) {
        cleanup_garbage(rstd::time::Instant::now(), true);
        current = rstd_try(usage());
        over    = current.image.saturating_add(current.audio)
                      .saturating_add(current.metadata)
                      .saturating_add(required) > capacity;
    }
    Vec<core_queries::EvictionRow> ids;
    {
        auto row =
            rstd_try(db.query(over ? core_queries::eviction<true> : core_queries::eviction<false>));
        ids = rstd_try(liorm::sqlito::all_rows(row, core_queries::eviction_projection));
    }
    rstd_try(evict_packs(required));
    rstd_try(evict_segments(required));
    for (const auto& entry : ids) {
        current    = rstd_try(usage());
        auto bytes = current.image.saturating_add(current.audio).saturating_add(current.metadata);
        if (entry.obsolete == i64() && bytes.saturating_add(required) <= capacity) continue;
        auto id = entry.id;
        if (auto value = versions.get(id))
            if ((*value)->lease.strong_count() > usize()) continue;
        if (auto cleanup = erase_version(id); cleanup.is_err())
            rstd::log::warn("cache deletion deferred: {}", rstd::move(cleanup).unwrap_err());
    }
    current = rstd_try(usage());
    if (current.image.saturating_add(current.audio)
            .saturating_add(current.metadata)
            .saturating_add(required) > capacity)
        return Err(Error::state("cache capacity reached"_str));
    return Ok(empty {});
}
} // namespace qcm::backend::resource_cache
