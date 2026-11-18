module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
module qcm.backend;
import :db.sync_batch.item_reference;
import :model.item_decode;
import :model.library;
import :db.codec;
import :db.schema;
import :db.timestamp;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace item_query = liorm::query;

namespace qcm::backend
{
auto ItemRef::validate(const ItemRef& value) -> Result<empty, ref<str>> {
    if (value.is_Key()) {
        switch (value.as_Key().value.type) {
        case ItemType::Album:
        case ItemType::Artist:
        case ItemType::Song:
        case ItemType::Mix:
        case ItemType::Radio: break;
        default: return Err("item.type is invalid"_str);
        }
    }
    return Ok(empty {});
}

namespace
{
auto contains_item_type(slice<ItemType> expected, ItemType type) -> bool {
    for (auto value : expected)
        if (value == type) return true;
    return false;
}
auto expected_item_type(slice<ItemType> expected, ItemType type) -> bool {
    return expected.is_empty() || contains_item_type(expected, type);
}
inline constexpr item_query::ProjectionLayout<item_query::ModelGroup<Library>> library_projection;
inline constexpr item_query::ProjectionLayout<item_query::ModelGroup<Item>>    item_projection;

inline constexpr auto find_library     = item_query::compile([](item_query::QueryBuilder& b) {
    auto library = b.use(database_tables).table<Library>();
    return b.select(item_query::project(library_projection, library).as_slice())
        .from(library.source())
        .where(library.col<&Library::id>() == b.param<i64>("id"_str));
});
inline constexpr auto find_item_id     = item_query::compile([](item_query::QueryBuilder& b) {
    auto item = b.use(database_tables).table<Item>();
    return b.select(item_query::project(item_projection, item).as_slice())
        .from(item.source())
        .where(item.col<&Item::id>() == b.param<i64>("id"_str));
});
inline constexpr auto find_unique_item = item_query::compile([](item_query::QueryBuilder& b) {
    auto item = b.use(database_tables).table<Item>();
    return b.select(item_query::project(item_projection, item).as_slice())
        .from(item.source())
        .where((item.col<&Item::provider_id>() == b.param<i64>("provider"_str)) &&
               (item.col<&Item::native_id>() == b.param<item_query::TextParameter>("native"_str)) &&
               (item.col<&Item::type>() == b.param<i32>("type"_str)))
        .limit(b.integer(2));
});
template<bool Global>
inline constexpr auto find_item_key = item_query::compile([](item_query::QueryBuilder& b) {
    auto item = b.use(database_tables).table<Item>();
    auto condition =
        (item.col<&Item::provider_id>() == b.param<i64>("provider"_str)) &&
        (item.col<&Item::native_id>() == b.param<item_query::TextParameter>("native"_str)) &&
        (item.col<&Item::type>() == b.param<i32>("type"_str));
    if constexpr (Global)
        condition = condition && item.col<&Item::library_id>().is_null();
    else
        condition = condition && (item.col<&Item::library_id>() == b.param<i64>("library"_str));
    return b.select(item_query::project(item_projection, item).as_slice())
        .from(item.source())
        .where(condition)
        .limit(b.integer(1));
});

template<class Database>
auto find_item_by_id(Database& tx, i64 id) -> DbWriteResult<Option<Item>> {
    return Ok(
        rstd_try(liorm::sqlito::fetch_model<find_item_id>(tx,
                                                          item_projection.group<0>(),
                                                          liorm::parameters(liorm::param<"id">(id)),
                                                          liorm::sqlito::RowConsumption::Drain)));
}
auto find_item_by_key(sqlito::Transaction& tx, i64 provider, const ItemKey& key,
                      Option<i64> library) -> DbWriteResult<Option<Item>> {
    // Preserve the original raw enum lookup semantics, including missing bad values.
    if (library.is_some())
        return Ok(rstd_try(liorm::sqlito::fetch_model<find_item_key<false>>(
            tx,
            item_projection.group<0>(),
            liorm::parameters(liorm::param<"provider">(provider),
                              liorm::param<"native">(key.native_id.as_str()),
                              liorm::param<"type">(i64(static_cast<int>(key.type))),
                              liorm::param<"library">(*library)))));
    return Ok(rstd_try(liorm::sqlito::fetch_model<find_item_key<true>>(
        tx,
        item_projection.group<0>(),
        liorm::parameters(liorm::param<"provider">(provider),
                          liorm::param<"native">(key.native_id.as_str()),
                          liorm::param<"type">(i64(static_cast<int>(key.type)))))));
}
auto owns_item_library(sqlito::Transaction& tx, i64 provider, i64 id) -> DbWriteResult<bool> {
    auto library =
        rstd_try(liorm::sqlito::fetch_model<find_library>(tx,
                                                          library_projection.group<0>(),
                                                          liorm::parameters(liorm::param<"id">(id)),
                                                          liorm::sqlito::RowConsumption::Drain));
    if (library.is_none()) return Ok(false);
    if (library->provider_id != provider)
        return Err(DbWriteError::Invalid(String::make("library belongs to another provider"_str)));
    return Ok(true);
}
auto item_library(sqlito::Transaction& tx, i64 provider, const ItemKey& key)
    -> DbWriteResult<Option<Option<i64>>> {
    auto length = key.native_id.as_str().len();
    if (length == usize() || length > usize(4096))
        return Err(DbWriteError::Invalid(String::make("invalid native_id"_str)));
    if (key.library_id.is_missing())
        return Err(DbWriteError::Invalid(
            String::make("library_id is required; use null for global items"_str)));
    auto library = key.library_id.value();
    if (library.is_some()) {
        if (! rstd_try(owns_item_library(tx, provider, *library)))
            return Ok<Option<Option<i64>>>(None());
    } else if (key.type != ItemType::Mix && key.type != ItemType::Radio) {
        return Err(
            DbWriteError::Invalid(String::make("only mixes and radio queues may be global"_str)));
    }
    return Ok(Some(rstd::move(library)));
}

// Preserve allocate's legacy write-then-decode failure timing. This policy is
// deliberately private to Item INSERT; all reads and ordinary writes retain the
// strict shared ItemType Codec. It never receives a Statement or transaction.
class ItemInsertWriter : public liorm::sqlito::Writer {
    using Base = liorm::sqlito::Writer;

public:
    explicit ItemInsertWriter(Base writer): Base(rstd::move(writer)) {}
    template<class T>
    auto write(usize slot, const T& value) -> Result<empty, sqlito::Error>
        requires(Base::supports<T>())
    {
        if constexpr (rstd::mtp::same_as<T, ItemType>)
            return Base::write<i64>(slot, i64(static_cast<int>(value)));
        else
            return Base::write<T>(slot, value);
    }
};
} // namespace

auto validate_library(sqlito::Transaction& tx, i64 provider, i64 id) -> DbWriteResult<empty> {
    if (! rstd_try(owns_item_library(tx, provider, id)))
        return Err(DbWriteError::Invalid(String::make("missing library"_str)));
    return Ok(empty {});
}

auto read_item_by_id(sqlito::Connection& db, i64 id) -> DbWriteResult<Option<Item>> {
    return find_item_by_id(db, id);
}

auto allocate_legacy_items(sqlito::Transaction& tx, i64 provider, slice<LegacyItemInput> inputs)
    -> DbWriteResult<Vec<i64>> {
    using C = liorm::Columns<Item>;
    Vec<i64> ids;
    for (const auto& input : inputs) {
        if (input.library_id.is_some() &&
            ! rstd_try(owns_item_library(tx, provider, *input.library_id)))
            return Err(DbWriteError::Invalid(String::make("missing library"_str)));
        ItemKey key;
        key.native_id = input.native_id.clone();
        key.type      = input.type;
        auto found    = rstd_try(find_item_by_key(tx, provider, key, input.library_id));
        if (found.is_some()) {
            liorm::Changeset<Item> changes { C::create_at    = input.create_at,
                                             C::update_at    = input.update_at,
                                             C::last_sync_at = input.last_sync_at };
            auto                   item = rstd_try(
                liorm::sqlito::update<database_tables>(tx, found->id, rstd::move(changes)));
            ids.push(i64(item.id));
        } else {
            liorm::InsertValues<Item> values { C::native_id    = input.native_id.clone(),
                                               C::library_id   = input.library_id,
                                               C::provider_id  = provider,
                                               C::type         = input.type,
                                               C::create_at    = input.create_at,
                                               C::update_at    = input.update_at,
                                               C::last_sync_at = input.last_sync_at };
            auto                      item = rstd_try(
                (liorm::sqlito::insert<database_tables, ItemInsertWriter>(tx, rstd::move(values))));
            ids.push(i64(item.id));
        }
    }
    return Ok(rstd::move(ids));
}

auto resolve_unique(sqlito::Connection& db, i64 provider, ref<str> native_id, ItemType type)
    -> DbWriteResult<Item> {
    auto stmt = rstd_try(liorm::sqlito::prepare<find_unique_item>(
        db,
        liorm::parameters(liorm::param<"provider">(provider),
                          liorm::param<"native">(native_id),
                          liorm::param<"type">(i32(static_cast<int>(type))))));
    if (rstd_try(stmt.step()) == sqlito::Step::Done)
        return Err(DbWriteError::Invalid(String::make("missing item reference"_str)));
    auto reader = stmt.reader();
    auto result = rstd_try(liorm::decode_model(item_projection.group<0>(), reader));
    if (rstd_try(stmt.step()) != sqlito::Step::Done)
        return Err(
            DbWriteError::Invalid(String::make("ambiguous item reference; supply a library"_str)));
    return Ok(rstd::move(result));
}

auto resolve_optional(sqlito::Transaction& tx, i64 provider, const ItemRef& reference,
                      slice<ItemType> expected) -> DbWriteResult<Option<Item>> {
    Option<Item> found;
    RSTD_MATCH(reference) {
        RSTD_CASE(Id, id) { found = rstd_try(find_item_by_id(tx, id)); }
        RSTD_CASE(Key, key) {
            if (! expected_item_type(expected, key.type))
                return Err(
                    DbWriteError::Invalid(String::make("item reference has wrong type"_str)));
            auto library = rstd_try(item_library(tx, provider, key));
            if (library.is_none()) return Ok<Option<Item>>(None());
            found = rstd_try(find_item_by_key(tx, provider, key, *library));
            if (found.is_some() && key.id.is_some() && found->id != *key.id)
                return Err(DbWriteError::Invalid(String::make("item id does not match key"_str)));
        }
    }
    if (found.is_none()) return Ok<Option<Item>>(None());
    if (found->provider_id != provider || ! expected_item_type(expected, found->type))
        return Err(
            DbWriteError::Invalid(String::make("item reference has wrong provider or type"_str)));
    return Ok(rstd::move(found));
}

auto resolve(sqlito::Transaction& tx, i64 provider, const ItemRef& reference,
             slice<ItemType> expected) -> DbWriteResult<Item> {
    auto found = rstd_try(resolve_optional(tx, provider, reference, expected));
    if (found.is_none())
        return Err(DbWriteError::Invalid(String::make("missing item reference"_str)));
    return Ok(rstd::move(found).unwrap());
}

auto allocate(sqlito::Transaction& tx, i64 provider, const ItemRef& reference,
              slice<ItemType> expected) -> DbWriteResult<Item> {
    using C = liorm::Columns<Item>;
    RSTD_MATCH(reference) {
        RSTD_CASE(Id) { return resolve(tx, provider, reference, expected); }
        RSTD_CASE(Key, key) {
            auto library = rstd_try(item_library(tx, provider, key));
            if (library.is_none())
                return Err(DbWriteError::Invalid(String::make("missing library"_str)));
            if (! contains_item_type(expected, key.type))
                return Err(DbWriteError::Invalid(String::make("wrong entity type"_str)));
            auto now   = Timestamp::now();
            auto found = rstd_try(find_item_by_key(tx, provider, key, *library));
            if (found.is_some()) {
                if (key.id.is_some() && *key.id != found->id)
                    return Err(
                        DbWriteError::Invalid(String::make("item id does not match key"_str)));
                liorm::Changeset<Item> changes { C::update_at = now, C::last_sync_at = now };
                return Ok(rstd_try(
                    liorm::sqlito::update<database_tables>(tx, found->id, rstd::move(changes))));
            }
            if (key.id.is_some())
                return Err(
                    DbWriteError::Invalid(String::make("supplied item id does not exist"_str)));
            liorm::InsertValues<Item> values { C::native_id    = key.native_id.clone(),
                                               C::library_id   = *library,
                                               C::provider_id  = provider,
                                               C::type         = key.type,
                                               C::create_at    = now,
                                               C::update_at    = now,
                                               C::last_sync_at = now };
            return Ok(rstd_try((
                liorm::sqlito::insert<database_tables, ItemInsertWriter>(tx, rstd::move(values)))));
        }
    }
}
} // namespace qcm::backend
