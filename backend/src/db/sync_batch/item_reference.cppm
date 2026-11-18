export module qcm.backend:db.sync_batch.item_reference;
export import :model.item;
export import :db.sync_batch.schema;
using namespace rstd::prelude;

export namespace qcm::backend
{
struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", decode_only)]] ItemKey {
    String native_id;
    [[pmacro::helper("serde::derive", default)]]
    Patch<Option<i64>> library_id;
    ItemType           type;
    [[pmacro::helper("serde::derive", default)]]
    Option<i64> id;
};
struct LegacyItemInput {
    String      native_id;
    Option<i64> library_id;
    ItemType    type;
    Timestamp   create_at, update_at, last_sync_at;
};
[[pmacro::invoke(
    "rstd::Enum", [[derive("serde::derive")]] [[helper("serde::derive", untagged, decode_only,
                                                       validate(ItemRef::validate))]] enum ItemRef {
                      Id(value : i64),
                      Key(value : ItemKey),
                  } members {
    public:
        static auto validate(const ItemRef& value) -> Result<empty, ref<str>>;
    })]];

auto validate_library(sqlito::Transaction& tx, i64 provider, i64 id) -> DbWriteResult<empty>;
auto read_item_by_id(sqlito::Connection& db, i64 id) -> DbWriteResult<Option<Item>>;
auto allocate_legacy_items(sqlito::Transaction& tx, i64 provider, slice<LegacyItemInput> inputs)
    -> DbWriteResult<Vec<i64>>;
auto resolve_unique(sqlito::Connection& db, i64 provider, ref<str> native_id, ItemType type)
    -> DbWriteResult<Item>;
auto resolve_optional(sqlito::Transaction& tx, i64 provider, const ItemRef& reference,
                      slice<ItemType> expected) -> DbWriteResult<Option<Item>>;
auto resolve(sqlito::Transaction& tx, i64 provider, const ItemRef& reference,
             slice<ItemType> expected) -> DbWriteResult<Item>;
// The caller owns the transaction and must roll it back after an error.
auto allocate(sqlito::Transaction& tx, i64 provider, const ItemRef& reference,
              slice<ItemType> expected) -> DbWriteResult<Item>;
} // namespace qcm::backend
