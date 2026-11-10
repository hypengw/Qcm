module;
#include <rstd/macro.hpp>
export module qcm.backend:model.item_decode;
import :model.item;
import :db.codec;
import liorm.sqlito;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
auto item_invalid_row() -> sqlito::Error {
    return sqlito::Error::from(sqlito::ErrorKind::Type, 0, String::make("invalid item row"_str));
}
auto decode_item(sqlito::Statement& stmt, usize offset, Option<ItemType> expected = None())
    -> Result<Item, sqlito::Error> {
    liorm::sqlito::ExpectedReader<ItemType> reader { stmt, offset, rstd::move(expected) };
    constexpr liorm::query::ProjectionLayout<liorm::query::ModelGroup<Item>> layout;
    return liorm::decode_model(layout.group<0>(), reader);
}
} // namespace qcm::backend
