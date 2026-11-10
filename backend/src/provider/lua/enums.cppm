module;
#include <rstd/macro.hpp>
export module qcm.backend:provider.lua.enums;
import :model.item;
import :model.album;
import :sync_state;
import rstd;
export import luato;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend::bindings
{
template<class T>
auto enum_table(luato::State& state) -> Result<luato::TableHandle, luato::Error> {
    auto output = rstd_try(state.create_table());
    for (auto value : rstd::enum_iter<T>()) {
        auto name    = rstd::format("{}", value);
        auto key     = luato::LuaValue::Bytes(rstd::move(name).into_bytes());
        auto integer = rstd::into<i64>(value);
        rstd_try(output.set(key, luato::LuaValue::Integer(integer)));
    }
    return Ok(rstd::move(output));
}
} // namespace qcm::backend::bindings

export namespace qcm::backend::bindings
{
auto install_enums(luato::State& state) -> Result<luato::TableHandle, luato::Error> {
    auto output = rstd_try(state.create_table());
    auto add    = [&]<class T>(ref<str> name) -> Result<empty, luato::Error> {
        auto table = rstd_try(enum_table<T>(state));
        auto key   = luato::LuaValue::Bytes(String::make(name).into_bytes());
        return output.set(key, luato::LuaValue::Table(rstd::move(table)));
    };
    rstd_try(add.template operator()<ItemType>("ItemType"_str));
    rstd_try(add.template operator()<AlbumType>("AlbumType"_str));
    rstd_try(add.template operator()<SyncState>("SyncState"_str));
    return Ok(rstd::move(output));
}
} // namespace qcm::backend::bindings
