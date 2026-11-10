module;
#include <rstd/macro.hpp>
export module qcm.backend:resource.objects;
import :resource.storage;
import :resource.pack;
import rstd.log;
using namespace rstd::prelude;
using namespace rstd::literals;
using rstd::tuple;
using rstd::bytes::Bytes;
using rstd::collections::HashMap;

namespace qcm::backend::resource_cache::objects_detail
{
namespace q = liorm::query;
using cache_db::ObjectRecord;
using cache_db::PackRecord;
struct ObjectRow {
    String  meta;
    i64     offset;
    i64     length;
    Vec<u8> digest;
    String  token;
    i64     committed;
};
inline constexpr liorm::RecordProjection<ObjectRow, &ObjectRow::meta, &ObjectRow::offset,
                                         &ObjectRow::length, &ObjectRow::digest, &ObjectRow::token,
                                         &ObjectRow::committed>
    object_projection;
struct AppendPackRow {
    String token;
    i64    length;
};
inline constexpr liorm::RecordProjection<AppendPackRow, &AppendPackRow::token,
                                         &AppendPackRow::length>
    append_pack_projection;
struct RetainedPackRow {
    String token;
    i64    length;
    i64    deleting;
};
inline constexpr liorm::RecordProjection<RetainedPackRow, &RetainedPackRow::token,
                                         &RetainedPackRow::length, &RetainedPackRow::deleting>
    retained_pack_projection;
struct PackCandidateRow {
    String token;
    i64    deleting;
};
inline constexpr liorm::RecordProjection<PackCandidateRow, &PackCandidateRow::token,
                                         &PackCandidateRow::deleting>
    pack_candidate_projection;

inline constexpr liorm::TupleProjection<String> pack_token_projection;

inline constexpr auto object              = q::compile([](q::QueryBuilder& b) {
    auto schema = b.use(cache_db::schema);
    auto object = schema.table<ObjectRecord>();
    auto pack   = schema.table<PackRecord>();
    return object_projection
        .select(b,
                object.col<&ObjectRecord::meta>(),
                object.col<&ObjectRecord::offset>(),
                object.col<&ObjectRecord::length>(),
                object.col<&ObjectRecord::digest>(),
                pack.col<&PackRecord::token>(),
                pack.col<&PackRecord::length>())
        .from(object.source())
        .join(pack.source(), object.col<&ObjectRecord::pack>() == pack.col<&PackRecord::token>())
        .where(
            (object.col<&ObjectRecord::resource_key>() == b.param<q::TextParameter>("key"_str)) &&
            (pack.col<&PackRecord::deleting>() == b.integer(0)));
});
inline constexpr auto delete_object       = q::compile([](q::QueryBuilder& b) {
    auto object = b.use(cache_db::schema).table<ObjectRecord>();
    return b.delete_from(object.source())
        .where(object.col<&ObjectRecord::resource_key>() == b.param<q::TextParameter>("key"_str));
});
inline constexpr auto touch_pack          = q::compile([](q::QueryBuilder& b) {
    auto pack = b.use(cache_db::schema).table<PackRecord>();
    return b.update(pack.source())
        .set(pack.col<&PackRecord::last_use>().set(b.param<i64>("clock"_str)))
        .where(pack.col<&PackRecord::token>() == b.param<q::TextParameter>("token"_str));
});
inline constexpr auto append_pack         = q::compile([](q::QueryBuilder& b) {
    auto pack = b.use(cache_db::schema).table<PackRecord>();
    return append_pack_projection
        .select(b, pack.col<&PackRecord::token>(), pack.col<&PackRecord::length>())
        .from(pack.source())
        .where((pack.col<&PackRecord::group_key>() == b.param<q::TextParameter>("group"_str)) &&
               (pack.col<&PackRecord::deleting>() == b.integer(0)) &&
               (pack.col<&PackRecord::length>() + b.param<i64>("length"_str) <=
                b.param<i64>("capacity"_str)))
        .order_by(pack.col<&PackRecord::last_use>().desc())
        .limit(b.integer(1));
});
inline constexpr auto insert_pack         = q::compile([](q::QueryBuilder& b) {
    auto pack = b.use(cache_db::schema).table<PackRecord>();
    return b
        .insert_into(pack.source(),
                     pack.col<&PackRecord::token>(),
                     pack.col<&PackRecord::group_key>(),
                     pack.col<&PackRecord::provider>(),
                     pack.col<&PackRecord::length>(),
                     pack.col<&PackRecord::last_use>())
        .values(b.param<q::TextParameter>("token"_str),
                b.param<q::TextParameter>("group"_str),
                b.param<i64>("provider"_str),
                b.param<i64>("length"_str),
                b.integer(0));
});
inline constexpr auto replace_object      = q::compile([](q::QueryBuilder& b) {
    auto object = b.use(cache_db::schema).table<ObjectRecord>();
    return b
        .insert_into(object.source(),
                     object.col<&ObjectRecord::resource_key>(),
                     object.col<&ObjectRecord::pack>(),
                     object.col<&ObjectRecord::offset>(),
                     object.col<&ObjectRecord::length>(),
                     object.col<&ObjectRecord::digest>(),
                     object.col<&ObjectRecord::meta>())
        .or_replace()
        .values(b.param<q::TextParameter>("key"_str),
                b.param<q::TextParameter>("pack"_str),
                b.param<i64>("offset"_str),
                b.param<i64>("length"_str),
                b.param<q::BlobParameter>("digest"_str),
                b.param<q::TextParameter>("meta"_str));
});
inline constexpr auto commit_pack         = q::compile([](q::QueryBuilder& b) {
    auto pack = b.use(cache_db::schema).table<PackRecord>();
    return b.update(pack.source())
        .set(pack.col<&PackRecord::length>().set(b.param<i64>("length"_str)),
             pack.col<&PackRecord::last_use>().set(b.param<i64>("clock"_str)))
        .where((pack.col<&PackRecord::token>() == b.param<q::TextParameter>("token"_str)) &&
               (pack.col<&PackRecord::deleting>() == b.integer(0)));
});
inline constexpr auto delete_pack_objects = q::compile([](q::QueryBuilder& b) {
    auto object = b.use(cache_db::schema).table<ObjectRecord>();
    return b.delete_from(object.source())
        .where(object.col<&ObjectRecord::pack>() == b.param<q::TextParameter>("token"_str));
});
inline constexpr auto mark_pack_deleting  = q::compile([](q::QueryBuilder& b) {
    auto pack = b.use(cache_db::schema).table<PackRecord>();
    return b.update(pack.source())
        .set(pack.col<&PackRecord::deleting>().set(b.integer(1)))
        .where(pack.col<&PackRecord::token>() == b.param<q::TextParameter>("token"_str));
});
inline constexpr auto delete_pack         = q::compile([](q::QueryBuilder& b) {
    auto pack = b.use(cache_db::schema).table<PackRecord>();
    return b.delete_from(pack.source())
        .where(pack.col<&PackRecord::token>() == b.param<q::TextParameter>("token"_str));
});
inline constexpr auto retained_packs      = q::compile([](q::QueryBuilder& b) {
    auto pack = b.use(cache_db::schema).table<PackRecord>();
    return retained_pack_projection
        .select(b,
                pack.col<&PackRecord::token>(),
                pack.col<&PackRecord::length>(),
                pack.col<&PackRecord::deleting>())
        .from(pack.source());
});
inline constexpr auto eviction_candidates = q::compile([](q::QueryBuilder& b) {
    auto pack = b.use(cache_db::schema).table<PackRecord>();
    return pack_candidate_projection
        .select(b, pack.col<&PackRecord::token>(), pack.col<&PackRecord::deleting>())
        .from(pack.source())
        .order_by(pack.col<&PackRecord::deleting>().desc(),
                  pack.col<&PackRecord::last_use>().asc());
});
inline constexpr auto provider_packs      = q::compile([](q::QueryBuilder& b) {
    auto pack = b.use(cache_db::schema).table<PackRecord>();
    return pack_token_projection.select(b, pack.col<&PackRecord::token>())
        .from(pack.source())
        .where(pack.col<&PackRecord::provider>() == b.param<i64>("provider"_str));
});
inline constexpr u64  HEADER { 64 };
auto                  name(ref<str> token) -> String { return rstd::format("{}.pack", token); }
auto                  unsigned_value(i64 value) -> DbResult<u64> {
    if (value < i64()) return Err(Error::state("negative packed object descriptor"_str));
    return Ok(u64(value.to_primitive()));
}
auto reservation(u64 length) -> u64 {
    return ((length + HEADER + u64(4095)) / u64(4096)) * u64(4096);
}
} // namespace qcm::backend::resource_cache::objects_detail

namespace qcm::backend::resource_cache
{
auto managed_pack_name(ref<str> value) -> bool {
    if (value.len() != usize(37) || ! value.ends_with(".pack"_str)) return false;
    return valid_generation(value.get(usize(), usize(32)).unwrap());
}
auto Storage::object(ref<str> key) -> DbResult<Option<Object>> {
    Option<ObjectMetadata> metadata;
    u64                    offset {}, length {}, committed {};
    Vec<u8>                digest;
    String                 pack_token;
    {
        auto row        = rstd_try(db.query(objects_detail::object, parameter("key"_str, key)));
        auto descriptor = rstd_try(liorm::sqlito::one_row(
            row, objects_detail::object_projection, liorm::sqlito::RowConsumption::First));
        if (descriptor.is_none()) return Ok(None());
        metadata   = Some(rstd_try(decode_object_metadata(descriptor->meta.as_str())));
        offset     = rstd_try(objects_detail::unsigned_value(descriptor->offset));
        length     = rstd_try(objects_detail::unsigned_value(descriptor->length));
        digest     = rstd::move(descriptor->digest);
        pack_token = rstd::move(descriptor->token);
        committed  = rstd_try(objects_detail::unsigned_value(descriptor->committed));
    }
    auto read = [&]() -> DbResult<Object> {
        if (! managed_pack_name(objects_detail::name(pack_token.as_str()).as_str()) ||
            offset < objects_detail::HEADER || length > MAX_OBJECT || metadata->length != length ||
            offset > committed || length > committed - offset || digest.len() != usize(32))
            return Err(Error::state("invalid packed object"_str));
        auto      pack = rstd_try(Pack::open(root.as_path(), pack_token.as_str(), committed));
        PackEntry entry { offset, length, {} };
        for (usize i {}; i < usize(32); ++i) entry.digest[i] = digest[i];
        auto bytes = rstd_try(pack.read(committed, entry));
        return Ok(Object { rstd::move(*metadata), Bytes::from_vec(rstd::move(bytes)) });
    }();
    if (read.is_err()) {
        rstd::log::warn("packed object discarded: {}", rstd::move(read).unwrap_err());
        rstd_try(db.execute(objects_detail::delete_object, parameter("key"_str, key)));
        return Ok(None());
    }
    auto clock = segment_tick();
    rstd_try(db.execute(objects_detail::touch_pack,
                        parameter("clock"_str, clock),
                        parameter("token"_str, pack_token.as_str())));
    return Ok(Some(rstd::move(read).unwrap()));
}
auto Storage::put_object(ObjectMetadata meta, slice<u8> bytes) -> DbResult<empty> {
    if (meta.length != u64(bytes.len().to_primitive()) || meta.length > MAX_OBJECT)
        return Err(Error::state("object admission limit"_str));
    rstd_try(evict(objects_detail::reservation(meta.length)));
    Option<tuple<String, u64>> existing;
    {
        auto row        = rstd_try(db.query(objects_detail::append_pack,
                                            parameter("group"_str, meta.group.as_str()),
                                            parameter("length"_str, meta.length),
                                            parameter("capacity"_str, PACK_BYTES)));
        auto descriptor = rstd_try(liorm::sqlito::one_row(
            row, objects_detail::append_pack_projection, liorm::sqlito::RowConsumption::First));
        if (descriptor.is_some()) {
            existing = Some(tuple<String, u64> {
                rstd::move(descriptor->token),
                rstd_try(objects_detail::unsigned_value(descriptor->length)) });
        }
    }
    String pack_token;
    u64    offset {};
    if (existing.is_some()) {
        pack_token = rstd::move(rstd::get<0>(*existing));
        offset     = rstd::get<1>(*existing);
    } else {
        pack_token   = rstd_try(token());
        auto path    = root.join(objects_detail::name(pack_token.as_str()).as_str());
        auto created = [&]() -> DbResult<empty> {
            auto pack = rstd_try(Pack::create(root.as_path(), pack_token.as_str()));
            rstd_try(sync_directory(root.as_path()));
            return Ok(rstd_try(db.execute(objects_detail::insert_pack,
                                          parameter("token"_str, pack_token.as_str()),
                                          parameter("group"_str, meta.group.as_str()),
                                          parameter("provider"_str, meta.provider),
                                          parameter("length"_str, objects_detail::HEADER))));
        }();
        auto allocation = allocated(path.as_path());
        u64  allocation_bytes { 4096 };
        if (allocation.is_ok())
            allocation_bytes = *allocation;
        else
            rstd::log::warn("pack allocation query failed: {}",
                            rstd::move(allocation).unwrap_err());
        sizes.insert(objects_detail::name(pack_token.as_str()),
                     { allocation_bytes, ResourceKind::Audio });
        if (created.is_err()) {
            if (auto cleanup = remove_managed(objects_detail::name(pack_token.as_str()).as_str());
                cleanup.is_err())
                rstd::log::warn("pack creation cleanup deferred: {}",
                                rstd::move(cleanup).unwrap_err());
            return created;
        }
        offset = objects_detail::HEADER;
    }
    if (! managed_pack_name(objects_detail::name(pack_token.as_str()).as_str()) ||
        offset < objects_detail::HEADER || offset > PACK_BYTES || meta.length > PACK_BYTES - offset)
        return Err(Error::state("invalid pack append"_str));
    auto path   = root.join(objects_detail::name(pack_token.as_str()).as_str());
    auto pack   = rstd_try(Pack::open(root.as_path(), pack_token.as_str(), offset));
    auto result = [&]() -> DbResult<empty> {
        auto entry   = rstd_try(pack.append(offset, bytes));
        auto clock   = segment_tick();
        auto encoded = rstd_try(encode_object_metadata(meta));
        return db.atomic([&]() -> DbResult<empty> {
            rstd_try(db.execute(objects_detail::replace_object,
                                parameter("key"_str, meta.key.as_str()),
                                parameter("pack"_str, pack_token.as_str()),
                                parameter("offset"_str, offset),
                                parameter("length"_str, meta.length),
                                parameter("digest"_str, entry.digest.as_slice()),
                                parameter("meta"_str, encoded.as_str())));
            return Ok(rstd_try(db.execute(objects_detail::commit_pack,
                                          parameter("length"_str, offset + meta.length),
                                          parameter("clock"_str, clock),
                                          parameter("token"_str, pack_token.as_str()))));
        });
    }();
    if (result.is_err())
        if (auto rollback = pack.recover(offset); rollback.is_err())
            rstd::log::warn("pack append rollback deferred: {}", rstd::move(rollback).unwrap_err());
    sizes.insert(objects_detail::name(pack_token.as_str()),
                 { rstd_try(allocated(path.as_path())), ResourceKind::Audio });
    return result;
}
auto Storage::delete_pack(ref<str> pack_token) -> DbResult<empty> {
    auto name = objects_detail::name(pack_token);
    if (! managed_pack_name(name.as_str())) return Err(Error::state("invalid pack token"_str));
    rstd_try(db.atomic([&]() -> DbResult<empty> {
        rstd_try(
            db.execute(objects_detail::delete_pack_objects, parameter("token"_str, pack_token)));
        return Ok(rstd_try(
            db.execute(objects_detail::mark_pack_deleting, parameter("token"_str, pack_token))));
    }));
    rstd_try(remove_managed(name.as_str()));
    return Ok(
        rstd_try(db.execute(objects_detail::delete_pack, parameter("token"_str, pack_token))));
}
auto Storage::retain_packs(HashMap<String, ResourceKind>& retained) -> DbResult<empty> {
    Vec<objects_detail::RetainedPackRow> packs;
    {
        auto rows = rstd_try(db.query(objects_detail::retained_packs));
        packs = rstd_try(liorm::sqlito::all_rows(rows, objects_detail::retained_pack_projection));
    }
    for (const auto& descriptor : packs) {
        const auto& pack_token = descriptor.token;
        auto        length     = descriptor.length;
        auto        deleting   = descriptor.deleting;
        auto        name       = objects_detail::name(pack_token.as_str());
        auto        valid      = [&]() -> DbResult<empty> {
            if (! managed_pack_name(name.as_str()) || length < i64(64) ||
                length > i64(PACK_BYTES.to_primitive()) || deleting != i64())
                return Err(Error::state("invalid pack descriptor"_str));
            auto committed = u64(length.to_primitive());
            auto pack      = rstd_try(Pack::open(root.as_path(), pack_token.as_str(), committed));
            return pack.recover(committed);
        }();
        if (valid.is_err())
            rstd::log::warn("invalid pack discarded: {}", rstd::move(valid).unwrap_err());
        if (valid.is_ok()) {
            retained.insert(rstd::move(name), ResourceKind::Audio);
        } else if (managed_pack_name(name.as_str())) {
            (void)delete_pack(pack_token.as_str());
        } else {
            rstd_try(db.execute(objects_detail::delete_pack_objects,
                                parameter("token"_str, pack_token.as_str())));
            rstd_try(db.execute(objects_detail::delete_pack,
                                parameter("token"_str, pack_token.as_str())));
        }
    }
    return Ok(empty {});
}
auto Storage::evict_packs(u64 required) -> DbResult<empty> {
    Vec<objects_detail::PackCandidateRow> packs;
    {
        auto rows = rstd_try(db.query(objects_detail::eviction_candidates));
        packs = rstd_try(liorm::sqlito::all_rows(rows, objects_detail::pack_candidate_projection));
    }
    for (const auto& descriptor : packs) {
        auto used = rstd_try(usage());
        if (descriptor.deleting == i64() && used.audio.saturating_add(used.image)
                                                    .saturating_add(used.metadata)
                                                    .saturating_add(required) <= capacity)
            break;
        if (auto cleanup = delete_pack(descriptor.token.as_str()); cleanup.is_err())
            rstd::log::warn("pack deletion deferred: {}", rstd::move(cleanup).unwrap_err());
    }
    return Ok(empty {});
}
auto Storage::invalidate_packs(i64 provider) -> DbResult<empty> {
    Vec<String> tokens;
    {
        auto rows =
            rstd_try(db.query(objects_detail::provider_packs, parameter("provider"_str, provider)));
        auto values =
            rstd_try(liorm::sqlito::all_rows(rows, objects_detail::pack_token_projection));
        for (auto& value : values) tokens.push(rstd::move(rstd::get<0>(value)));
    }
    for (const auto& pack_token : tokens)
        if (auto cleanup = delete_pack(pack_token.as_str()); cleanup.is_err())
            rstd::log::warn("pack invalidation deferred: {}", rstd::move(cleanup).unwrap_err());
    return Ok(empty {});
}
} // namespace qcm::backend::resource_cache
