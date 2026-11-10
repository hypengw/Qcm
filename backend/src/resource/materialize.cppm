module;
#include <rstd/macro.hpp>
export module qcm.backend:resource.materialize;
import :resource.storage;
import :resource.schema;
import liorm.sqlito;
import rstd.log;
using namespace rstd::prelude;
using namespace rstd::literals;
using rstd::fs::File;
using rstd::path::Path;
using rstd::path::PathBuf;
using rstd::sync::atomic::Ordering;

namespace qcm::backend::resource_cache::materialize_detail
{
namespace q = liorm::query;
using namespace cache_db;

struct PlainDescriptorRow {
    i64             ready;
    i64             length;
    Option<Vec<u8>> digest;
    i64             format;
};
inline constexpr liorm::RecordProjection<PlainDescriptorRow, &PlainDescriptorRow::ready,
                                         &PlainDescriptorRow::length, &PlainDescriptorRow::digest,
                                         &PlainDescriptorRow::format>
    plain_descriptor_projection;

inline constexpr liorm::TupleProjection<i64>    version_id_projection;
inline constexpr liorm::TupleProjection<String> generation_projection;

inline constexpr auto plain_descriptor_query  = q::compile([](q::QueryBuilder& b) {
    auto f = b.use(schema).table<CompleteFileRecord>();
    return plain_descriptor_projection
        .select(b,
                f.col<&CompleteFileRecord::ready>(),
                f.col<&CompleteFileRecord::length>(),
                f.col<&CompleteFileRecord::digest>(),
                f.col<&CompleteFileRecord::format>())
        .from(f.source())
        .where(f.col<&CompleteFileRecord::version_id>() == b.param<i64>("id"_str));
});
inline constexpr auto build_candidates_query  = q::compile([](q::QueryBuilder& b) {
    auto tables = b.use(schema);
    auto v      = tables.table<VersionRecord>("v"_str);
    auto f      = tables.table<CompleteFileRecord>("f"_str);
    auto ready =
        b.function(q::Function::Coalesce, f.col<&CompleteFileRecord::ready>(), b.integer(0));
    auto retry =
        b.function(q::Function::Coalesce, f.col<&CompleteFileRecord::retry_at>(), b.integer(0));
    return version_id_projection.select(b, v.col<&VersionRecord::id>())
        .from(v.source())
        .left_join(f.source(),
                   f.col<&CompleteFileRecord::version_id>() == v.col<&VersionRecord::id>())
        .where((v.col<&VersionRecord::obsolete>() == b.integer(0)) && (ready == b.integer(0)) &&
               (retry <= b.param<i64>("now"_str)) &&
               (v.col<&VersionRecord::id>() > b.param<i64>("cursor"_str)))
        .order_by(v.col<&VersionRecord::id>())
        .limit(b.integer(64));
});
inline constexpr auto version_state_query     = q::compile([](q::QueryBuilder& b) {
    auto v = b.use(schema).table<VersionRecord>();
    return b.select(v.col<&VersionRecord::obsolete>())
        .from(v.source())
        .where(v.col<&VersionRecord::id>() == b.param<i64>("id"_str));
});
inline constexpr auto publish_file_query      = q::compile([](q::QueryBuilder& b) {
    auto f = b.use(schema).table<CompleteFileRecord>();
    return b
        .insert_into(f.source(),
                     f.col<&CompleteFileRecord::version_id>(),
                     f.col<&CompleteFileRecord::ready>(),
                     f.col<&CompleteFileRecord::length>(),
                     f.col<&CompleteFileRecord::digest>(),
                     f.col<&CompleteFileRecord::retry_at>())
        .values(b.param<i64>("id"_str),
                b.integer(1),
                b.param<i64>("length"_str),
                b.param<q::BlobParameter>("digest"_str),
                b.integer(0))
        .on_conflict_do_update(
            array<q::Expr, 1> { f.col<&CompleteFileRecord::version_id>() }.as_slice(),
            array<q::Expr, 4> { f.col<&CompleteFileRecord::ready>().set(b.integer(1)),
                                f.col<&CompleteFileRecord::length>().set(
                                    b.excluded(f.col<&CompleteFileRecord::length>())),
                                f.col<&CompleteFileRecord::digest>().set(
                                    b.excluded(f.col<&CompleteFileRecord::digest>())),
                                f.col<&CompleteFileRecord::retry_at>().set(b.integer(0)) }
                .as_slice());
});
inline constexpr auto defer_build_query       = q::compile([](q::QueryBuilder& b) {
    auto f = b.use(schema).table<CompleteFileRecord>();
    return b
        .insert_into(f.source(),
                     f.col<&CompleteFileRecord::version_id>(),
                     f.col<&CompleteFileRecord::retry_at>())
        .values(b.param<i64>("id"_str), b.param<i64>("retry_at"_str))
        .on_conflict_do_update(
            array<q::Expr, 1> { f.col<&CompleteFileRecord::version_id>() }.as_slice(),
            array<q::Expr, 1> { f.col<&CompleteFileRecord::retry_at>().set(
                                    b.excluded(f.col<&CompleteFileRecord::retry_at>())) }
                .as_slice());
});
inline constexpr auto plain_generations_query = q::compile([](q::QueryBuilder& b) {
    auto tables = b.use(schema);
    auto v      = tables.table<VersionRecord>();
    auto f      = tables.table<CompleteFileRecord>();
    return generation_projection.select(b, v.col<&VersionRecord::generation>())
        .from(v.source())
        .join(f.source(), f.col<&CompleteFileRecord::version_id>() == v.col<&VersionRecord::id>())
        .where(f.col<&CompleteFileRecord::ready>() == b.integer(1));
});
inline constexpr auto retire_corrupt_query    = q::compile([](q::QueryBuilder& b) {
    auto v = b.use(schema).table<VersionRecord>();
    return b.update(v.source())
        .set(v.col<&VersionRecord::obsolete>().set(b.integer(2)))
        .where(v.col<&VersionRecord::id>() == b.param<i64>("id"_str));
});
inline constexpr auto file_clock_query        = q::compile([](q::QueryBuilder& b) {
    auto c = b.use(schema).table<FileClockRecord>();
    return b.select(c.col<&FileClockRecord::time>())
        .from(c.source())
        .where(c.col<&FileClockRecord::id>() == b.integer(1));
});
inline constexpr auto any_lease_query         = q::compile([](q::QueryBuilder& b) {
    auto l = b.use(schema).table<FileLeaseRecord>();
    return b.select(b.integer(1)).from(l.source()).limit(b.integer(1));
});
inline constexpr auto persist_clock_query     = q::compile([](q::QueryBuilder& b) {
    auto c = b.use(schema).table<FileClockRecord>();
    return b.insert_into(c.source(), c.col<&FileClockRecord::id>(), c.col<&FileClockRecord::time>())
        .values(b.integer(1), b.param<i64>("now"_str))
        .on_conflict_do_update(
            array<q::Expr, 1> { c.col<&FileClockRecord::id>() }.as_slice(),
            array<q::Expr, 1> { c.col<&FileClockRecord::time>().set(
                                    b.function(q::Function::Max,
                                               c.col<&FileClockRecord::time>(),
                                               b.excluded(c.col<&FileClockRecord::time>()))) }
                .as_slice());
});
inline constexpr auto expire_leases_query     = q::compile([](q::QueryBuilder& b) {
    auto l = b.use(schema).table<FileLeaseRecord>();
    return b.delete_from(l.source())
        .where(l.col<&FileLeaseRecord::expires>() <= b.param<i64>("now"_str));
});
inline constexpr auto version_lease_query     = q::compile([](q::QueryBuilder& b) {
    auto l = b.use(schema).table<FileLeaseRecord>();
    return b.select(b.integer(1))
        .from(l.source())
        .where(l.col<&FileLeaseRecord::version_id>() == b.param<i64>("id"_str))
        .limit(b.integer(1));
});
inline constexpr auto lease_count_query       = q::compile([](q::QueryBuilder& b) {
    auto l = b.use(schema).table<FileLeaseRecord>();
    return b.select(b.function(q::Function::Count, b.star())).from(l.source());
});
inline constexpr auto protected_bytes_query   = q::compile([](q::QueryBuilder& b) {
    auto tables = b.use(schema);
    auto f      = tables.table<CompleteFileRecord>();
    auto l      = tables.table<FileLeaseRecord>();
    auto leased = b.select(l.col<&FileLeaseRecord::version_id>())
                      .distinct()
                      .from(l.source())
                      .where(l.col<&FileLeaseRecord::version_id>() != b.param<i64>("id"_str));
    return b
        .select(b.function(q::Function::Coalesce,
                           b.function(q::Function::Sum, f.col<&CompleteFileRecord::length>()),
                           b.integer(0)))
        .from(f.source())
        .where(f.col<&CompleteFileRecord::version_id>().in(leased));
});
inline constexpr auto acquire_lease_query     = q::compile([](q::QueryBuilder& b) {
    auto l = b.use(schema).table<FileLeaseRecord>();
    return b
        .insert_into(l.source(),
                     l.col<&FileLeaseRecord::token>(),
                     l.col<&FileLeaseRecord::version_id>(),
                     l.col<&FileLeaseRecord::client>(),
                     l.col<&FileLeaseRecord::expires>())
        .values(b.param<q::TextParameter>("token"_str),
                b.param<i64>("id"_str),
                b.param<q::TextParameter>("client"_str),
                b.param<i64>("expires"_str));
});
inline constexpr auto renewable_version_query = q::compile([](q::QueryBuilder& b) {
    auto tables = b.use(schema);
    auto l      = tables.table<FileLeaseRecord>("l"_str);
    auto v      = tables.table<VersionRecord>("v"_str);
    return b.select(l.col<&FileLeaseRecord::version_id>())
        .from(l.source())
        .join(v.source(), v.col<&VersionRecord::id>() == l.col<&FileLeaseRecord::version_id>())
        .where((l.col<&FileLeaseRecord::token>() == b.param<q::TextParameter>("token"_str)) &&
               (l.col<&FileLeaseRecord::client>() == b.param<q::TextParameter>("client"_str)) &&
               (v.col<&VersionRecord::obsolete>() == b.integer(0)) &&
               (l.col<&FileLeaseRecord::expires>() > b.param<i64>("now"_str)));
});
inline constexpr auto renew_lease_query       = q::compile([](q::QueryBuilder& b) {
    auto l = b.use(schema).table<FileLeaseRecord>();
    return b.update(l.source())
        .set(l.col<&FileLeaseRecord::expires>().set(b.param<i64>("expires"_str)))
        .where((l.col<&FileLeaseRecord::token>() == b.param<q::TextParameter>("token"_str)) &&
               (l.col<&FileLeaseRecord::client>() == b.param<q::TextParameter>("client"_str)));
});

auto reservation(u64 length) -> u64 {
    return (length / u64(4096) + (length % u64(4096) != u64() ? u64(1) : u64())) * u64(4096);
}
} // namespace qcm::backend::resource_cache::materialize_detail

namespace qcm::backend::resource_cache
{
auto private_directory(ref<Path> path) -> DbResult<empty> {
#ifndef _WIN32
    return Ok(
        rstd_try(rstd::fs::set_permissions(path, rstd::fs::Permissions::from_mode(u32(0700)))));
#else
    (void)path;
    return Ok(empty {});
#endif
}
auto private_file(const File& file) -> DbResult<empty> {
#ifndef _WIN32
    auto handle = rstd_try(file.try_clone());
    return Ok(rstd_try(handle.set_permissions(rstd::fs::Permissions::from_mode(u32(0600)))));
#else
    (void)file;
    return Ok(empty {});
#endif
}
auto regular_file(ref<Path> path) -> DbResult<empty> {
    auto metadata = rstd_try(rstd::fs::symlink_metadata(path));
    if (! metadata.file_type().is_file()) return Err(Error::state("invalid cache file type"_str));
    return Ok(empty {});
}
auto managed_file_name(ref<str> name) -> bool {
    for (auto suffix : array<ref<str>, 3> { ".data"_str, ".ok.tmp"_str, ".ok"_str }) {
        if (name.ends_with(suffix) &&
            valid_generation(name.get(usize(), name.len() - suffix.len()).unwrap()))
            return true;
    }
    return false;
}
auto Storage::copied_block(i64 id, u64 bytes) -> DbResult<empty> {
    rstd_try(check_build(id));
    auto value = rstd_try(load(id));
    auto name  = rstd::format("{}.ok.tmp", value->meta.generation.as_str());
    sizes.insert(rstd::move(name), { bytes, value->meta.kind });
    building->remaining =
        materialize_detail::reservation(value->meta.geometry.length).saturating_sub(bytes);
    return Ok(empty {});
}
auto Storage::file_path(ref<str> generation, ref<str> suffix) const -> PathBuf {
    return root.join(rstd::format("{}.{}", generation, suffix).as_str());
}
auto Storage::plain(i64 id) -> DbResult<bool> { return Ok(rstd_try(plain_length(id)).is_some()); }
auto Storage::plain_length(i64 id) -> DbResult<Option<u64>> {
    auto row =
        rstd_try(db.query(materialize_detail::plain_descriptor_query, parameter("id"_str, id)));
    if (! rstd_try(next(row))) return Ok(None());
    auto reader = row.reader();
    if (rstd_try(materialize_detail::plain_descriptor_projection
                     .field<&materialize_detail::PlainDescriptorRow::ready>(reader)) != i64(1))
        return Ok(None());
    auto descriptor = rstd_try(materialize_detail::plain_descriptor_projection.decode(reader));
    if (descriptor.length < i64() || descriptor.digest.is_none() ||
        descriptor.digest->len() != usize(32) || descriptor.format != i64(1))
        return Err(Error::state("invalid plain descriptor"_str));
    return Ok(Some(u64(descriptor.length.to_primitive())));
}
auto Storage::next_build() -> DbResult<Option<Ticket>> {
    if (! files_enabled) return Ok(None());
    rstd_try(expire_leases());
    cleanup_garbage();
    rstd_try(cleanup_plain_data());
    if (auto maintenance = evict(u64()); maintenance.is_err())
        rstd::log::debug("cache maintenance waiting: {}", rstd::move(maintenance).unwrap_err());
    Vec<i64> ids;
    {
        auto rows = rstd_try(db.query(materialize_detail::build_candidates_query,
                                      parameter("now"_str, lease_now()),
                                      parameter("cursor"_str, scan_cursor)));
        auto values =
            rstd_try(liorm::sqlito::all_rows(rows, materialize_detail::version_id_projection));
        for (auto& value : values) ids.push(rstd::move(rstd::get<0>(value)));
    }
    if (ids.is_empty()) {
        scan_cursor = i64();
        return Ok(None());
    }
    for (auto id : ids) {
        scan_cursor = id;
        auto loaded = load(id);
        if (loaded.is_err()) {
            rstd::log::warn("file candidate unavailable: {}", rstd::move(loaded).unwrap_err());
            continue;
        }
        auto& value = **loaded;
        if (value.meta.layout.segment_bytes().is_some() ||
            (value.meta.kind == ResourceKind::Audio &&
             value.meta.geometry.length > SEGMENT_BYTES) ||
            ! value.meta.policy.store || ! value.index.is_committed() || ! value.index.complete())
            continue;
        auto version = rstd_try(this->version(id));
        auto clean   = [&]() -> DbResult<empty> {
            for (auto suffix : array<ref<str>, 2> { "ok.tmp"_str, "ok"_str })
                rstd_try(remove_managed(
                    rstd::format("{}.{}", version.meta.generation.as_str(), suffix).as_str()));
            return Ok(empty {});
        };
        if (auto cleanup = clean(); cleanup.is_err()) {
            rstd::log::warn("unpublished file cleanup deferred: {}",
                            rstd::move(cleanup).unwrap_err());
            rstd_try(defer_build(id));
            continue;
        }
        auto reserved = materialize_detail::reservation(version.meta.geometry.length);
        if (auto admission = evict(reserved); admission.is_err()) {
            rstd::log::debug("file conversion waiting for capacity: {}",
                             rstd::move(admission).unwrap_err());
            rstd_try(defer_build(id));
            continue;
        }
        auto prepare = [&]() -> DbResult<File> {
            rstd_try(clean());
            auto file = rstd_try(File::options().write(true).read(true).create_new(true).open(
                file_path(version.meta.generation.as_str(), "ok.tmp"_str).as_path()));
            rstd_try(private_file(file));
            return Ok(rstd::move(file));
        };
        auto prepared = prepare();
        if (prepared.is_err()) {
            rstd::log::warn("cache file creation deferred: {}", rstd::move(prepared).unwrap_err());
            rstd_try(defer_build(id));
            continue;
        }
        auto file  = rstd::move(prepared).unwrap();
        auto bytes = rstd_try(allocated(file));
        sizes.insert(rstd::format("{}.ok.tmp", version.meta.generation.as_str()),
                     { bytes, version.meta.kind });
        building = Some(Build { id, reserved });
        return Ok(Some(Ticket { rstd::move(version), rstd::move(file) }));
    }
    return Ok(None());
}
auto Storage::check_build(i64 id) -> DbResult<empty> {
    if (building.is_none() || building->id != id) return Err(Error::state("stale file ticket"_str));
    auto row = rstd_try(db.query(materialize_detail::version_state_query, parameter("id"_str, id)));
    auto obsolete = rstd_try(liorm::sqlito::optional_scalar<i64>(row));
    if (obsolete.is_none() || *obsolete != i64()) return Err(Error::invalidated());
    return Ok(empty {});
}
auto Storage::publish_file(Version version, array<u8, 32> digest) -> DbResult<empty> {
    rstd_try(check_build(version.id));
    if (! version.is_valid()) return Err(Error::invalidated());
    auto value = rstd_try(load(version.id));
    if (! value->index.is_committed() || ! value->index.complete() || value->plain)
        return Err(Error::state("file index not committed"_str));
    auto temp       = file_path(version.meta.generation.as_str(), "ok.tmp"_str);
    auto final_path = file_path(version.meta.generation.as_str(), "ok"_str);
    rstd_try(regular_file(temp.as_path()));
    auto metadata = rstd_try(rstd::fs::metadata(temp.as_path()));
    if (metadata.len() != version.meta.geometry.length)
        return Err(Error::state("file output size changed"_str));
    auto exists = rstd_try(rstd::fs::exists(final_path.as_path()));
    if (exists) return Err(Error::state("file already published"_str));
    rstd_try(rstd::fs::rename(temp.as_path(), final_path.as_path()));
    sizes.remove(rstd::format("{}.ok.tmp", version.meta.generation.as_str()));
    sizes.insert(rstd::format("{}.ok", version.meta.generation.as_str()),
                 { rstd_try(allocated(final_path.as_path())), version.meta.kind });
    if (building.is_some()) building->remaining = u64();
    rstd_try(sync_directory(root.as_path()));
    auto file = rstd_try(File::open(final_path.as_path()));
    rstd_try(db.atomic([&]() -> DbResult<empty> {
        return Ok(rstd_try(db.execute(materialize_detail::publish_file_query,
                                      parameter("id"_str, version.id),
                                      parameter("length"_str, version.meta.geometry.length),
                                      parameter("digest"_str, digest.as_slice()))));
    }));
    auto resident = versions.get_mut(version.id);
    if (resident.is_none()) return Err(Error::state("published version not resident"_str));
    (*resident)->file  = Some(rstd::move(file));
    (*resident)->plain = true;
    building           = None();
    if (auto cleanup = cleanup_plain_data(); cleanup.is_err())
        rstd::log::warn("published file cleanup deferred: {}", rstd::move(cleanup).unwrap_err());
    return Ok(empty {});
}
auto Storage::defer_build(i64 id) -> DbResult<empty> {
    return Ok(rstd_try(db.execute(materialize_detail::defer_build_query,
                                  parameter("id"_str, id),
                                  parameter("retry_at"_str, lease_now() + u64(30)))));
}
auto Storage::finish_failed(i64 id) -> DbResult<empty> {
    building = None();
    rstd_try(defer_build(id));
    auto value      = rstd_try(load(id));
    auto generation = value->meta.generation.clone();
    auto kind       = value->meta.kind;
    if (! rstd_try(plain(id))) {
        for (auto suffix : array<ref<str>, 2> { "ok.tmp"_str, "ok"_str }) {
            auto name = rstd::format("{}.{}", generation.as_str(), suffix);
            auto path = root.join(name.as_str());
            if (rstd::fs::symlink_metadata(path.as_path()).is_ok())
                sizes.insert(name.clone(), { rstd_try(allocated(path.as_path())), kind });
            rstd_try(remove_managed(name.as_str()));
        }
    }
    return Ok(empty {});
}
auto Storage::cleanup_plain_data() -> DbResult<empty> {
    Vec<String> names;
    {
        auto rows = rstd_try(db.query(materialize_detail::plain_generations_query));
        auto values =
            rstd_try(liorm::sqlito::all_rows(rows, materialize_detail::generation_projection));
        for (const auto& value : values)
            names.push(rstd::format("{}.data", rstd::get<0>(value).as_str()));
    }
    for (const auto& name : names)
        if (sizes.contains_key(name)) rstd_try(remove_managed(name.as_str()));
    return Ok(empty {});
}
auto Storage::remove_managed(ref<str> name) -> DbResult<empty> {
    if (! managed_file_name(name) && ! managed_segment_name(name) && ! managed_pack_name(name))
        return Err(Error::state("unmanaged cache path"_str));
    auto result = [&]() -> DbResult<empty> {
        auto removed = rstd::fs::remove_file(root.join(name).as_path());
        if (removed.is_err()) {
            auto cause = rstd::move(removed).unwrap_err();
            if (cause.kind().code != rstd::io::ErrorKind::NotFound)
                return Err(Error::io(rstd::move(cause)));
        }
        return sync_directory(root.as_path());
    }();
    if (result.is_err()) {
        garbage.insert(String::make(name));
        return result;
    }
    garbage.remove(name);
    sizes.remove(name);
    return Ok(empty {});
}
void Storage::cleanup_garbage() {
    Vec<String> names;
    for (auto name : garbage.iter()) {
        if (names.len() == usize(64)) break;
        names.push(name->clone());
    }
    for (const auto& name : names)
        if (auto cleanup = remove_managed(name.as_str()); cleanup.is_err())
            rstd::log::debug("cache cleanup still deferred: {}", rstd::move(cleanup).unwrap_err());
}
auto Storage::retire_corrupt(i64 id) -> DbResult<empty> {
    if (auto value = versions.get(id))
        if (auto lease = (*value)->lease.upgrade()) lease->valid.store(false, Ordering::Release);
    return Ok(
        rstd_try(db.execute(materialize_detail::retire_corrupt_query, parameter("id"_str, id))));
}
auto Storage::lease_now() const -> u64 { return clock.saturating_add(started.elapsed().as_secs()); }
auto Storage::restore_leases() -> DbResult<empty> {
    u64 old {};
    {
        auto row    = rstd_try(db.query(materialize_detail::file_clock_query));
        auto stored = rstd_try(liorm::sqlito::optional_scalar<i64>(row));
        if (stored.is_some()) old = u64(stored->to_primitive());
    }
    clock_uncertain = clock < old;
    clock           = rstd::cmp::max(clock, old);
    {
        auto row = rstd_try(db.query(materialize_detail::any_lease_query));
        if (rstd_try(next(row)))
            restart_grace =
                Some(rstd::time::Instant::now() + rstd::time::Duration::from_secs(LEASE_SECONDS));
    }
    return persist_clock();
}
auto Storage::persist_clock() -> DbResult<empty> {
    return Ok(rstd_try(
        db.execute(materialize_detail::persist_clock_query, parameter("now"_str, lease_now()))));
}
auto Storage::expire_leases() -> DbResult<empty> {
    if (now() < clock) clock_uncertain = true;
    if (clock_uncertain && now() >= lease_now()) clock_uncertain = false;
    if (! clock_uncertain &&
        (restart_grace.is_none() || rstd::time::Instant::now() >= *restart_grace))
        rstd_try(
            db.execute(materialize_detail::expire_leases_query, parameter("now"_str, lease_now())));
    return Ok(empty {});
}
auto Storage::externally_pinned(i64 id) -> DbResult<bool> {
    rstd_try(expire_leases());
    auto row = rstd_try(db.query(materialize_detail::version_lease_query, parameter("id"_str, id)));
    return Ok(rstd_try(next(row)));
}
auto Storage::acquire_file(i64 id, ref<str> client) -> DbResult<Option<LocalFile>> {
    rstd_try(expire_leases());
    if (clock_uncertain || ! rstd_try(plain(id))) return Ok(None());
    {
        auto row =
            rstd_try(db.query(materialize_detail::version_state_query, parameter("id"_str, id)));
        auto obsolete = rstd_try(liorm::sqlito::optional_scalar<i64>(row));
        if (obsolete.is_none() || *obsolete != i64()) return Ok(None());
    }
    auto value      = rstd_try(load(id));
    auto generation = value->meta.generation.clone();
    auto length     = value->meta.geometry.length;
    if (value->meta.kind != ResourceKind::Image) return Ok(None());
    auto path    = file_path(generation.as_str(), "ok"_str);
    auto regular = regular_file(path.as_path());
    bool corrupt = regular.is_err();
    if (corrupt)
        rstd::log::warn("published file validation failed: {}", rstd::move(regular).unwrap_err());
    if (! corrupt) {
        auto metadata = rstd_try(rstd::fs::metadata(path.as_path()));
        corrupt       = metadata.len() != length;
    }
    if (corrupt) {
        rstd_try(retire_corrupt(id));
        return Err(Error::plain_corrupt());
    }
    {
        auto row = rstd_try(db.query(materialize_detail::lease_count_query));
        if (rstd_try(liorm::sqlito::scalar<i64>(row)) >= i64(256)) return Ok(None());
    }
    u64 protected_bytes {};
    {
        auto row =
            rstd_try(db.query(materialize_detail::protected_bytes_query, parameter("id"_str, id)));
        protected_bytes = u64(rstd_try(liorm::sqlito::scalar<i64>(row)).to_primitive());
    }
    if (protected_bytes.saturating_add(length) >
        rstd::cmp::min(capacity / u64(2), u64(512 * 1024 * 1024)))
        return Ok(None());
    auto lease_token = rstd_try(token());
    rstd_try(persist_clock());
    rstd_try(db.execute(materialize_detail::acquire_lease_query,
                        parameter("token"_str, lease_token.as_str()),
                        parameter("id"_str, id),
                        parameter("client"_str, client),
                        parameter("expires"_str, lease_now() + LEASE_SECONDS)));
    return Ok(Some(LocalFile {
        rstd::move(path), rstd::move(lease_token), LEASE_SECONDS, rstd::move(generation) }));
}
auto Storage::renew_file(ref<str> lease_token, ref<str> client) -> DbResult<bool> {
    rstd_try(expire_leases());
    if (clock_uncertain) return Ok(false);
    i64 id {};
    {
        auto row    = rstd_try(db.query(materialize_detail::renewable_version_query,
                                        parameter("token"_str, lease_token),
                                        parameter("client"_str, client),
                                        parameter("now"_str, lease_now())));
        auto leased = rstd_try(liorm::sqlito::optional_scalar<i64>(row));
        if (leased.is_none()) return Ok(false);
        id = *leased;
    }
    auto value      = rstd_try(load(id));
    auto generation = value->meta.generation.clone();
    auto length     = value->meta.geometry.length;
    auto path       = file_path(generation.as_str(), "ok"_str);
    auto regular    = regular_file(path.as_path());
    bool corrupt    = regular.is_err();
    if (corrupt)
        rstd::log::warn("published file validation failed: {}", rstd::move(regular).unwrap_err());
    if (! corrupt) {
        auto metadata = rstd_try(rstd::fs::metadata(path.as_path()));
        corrupt       = metadata.len() != length;
    }
    if (corrupt) {
        rstd_try(retire_corrupt(id));
        return Ok(false);
    }
    rstd_try(persist_clock());
    rstd_try(db.execute(materialize_detail::renew_lease_query,
                        parameter("expires"_str, lease_now() + LEASE_SECONDS),
                        parameter("token"_str, lease_token),
                        parameter("client"_str, client)));
    return Ok(true);
}
} // namespace qcm::backend::resource_cache
