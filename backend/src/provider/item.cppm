module;
#include <rstd/macro.hpp>
export module qcm.backend:provider.item;
export import :error;
export import :provider.runtime;
export import :db.provider.item;
import :db.database;
using alloc::rc::Rc;
using namespace rstd::prelude;

export namespace qcm::backend
{
struct ProviderRequestOwner {
    i64            provider_id {};
    ProviderHandle instance;
};
using ProviderRequestOwnerHandle = Rc<ProviderRequestOwner>;

auto resolve_provider_item(sqlito::ExecutorHandle db, ProviderRuntime& runtime, i64 id, i64 type)
    -> rstd::async::coro<ProcessResult<Option<ProviderItem>>> {
    if (id <= i64()) co_return Err(ProcessError::WrongId(rstd::format("{}", id)));
    auto kind = rstd_co_try(rstd::try_into<ItemType>(type),
                            ProcessError::NoSuchItemType(rstd::format("{}", _e.value)));
    if (kind == ItemType::AlbumArtist) kind = ItemType::Artist;
    if (kind != ItemType::Song && kind != ItemType::Album && kind != ItemType::Artist &&
        kind != ItemType::Mix)
        co_return Ok(None());
    ProviderItemRead request { id, kind };
    auto             item = rstd_co_try(co_await database_run(rstd::move(db), rstd::move(request)));
    if (item.is_some() && runtime.find(item->provider_id) == nullptr)
        co_return Err(ProcessError::NoSuchProvider(rstd::format("{}", item->provider_id)));
    co_return Ok(rstd::move(item));
}
} // namespace qcm::backend
