#include "qcm_interface/sync_api.h"
#include "qcm_interface/global.h"
#include "qcm_interface/notifier.h"

#include "qcm_interface/action.h"
#include "qcm_interface/sql/item_sql.h"

namespace qcm::query
{

auto SyncAPi::sync_library_list(i64 providerId) -> task<Result<bool>> {
    if (auto c = Global::instance()->client(providerId)) {
        co_return co_await c->api->sync_library_list(*c);
    }
    co_return false;
}
auto SyncAPi::sync_collection(enums::CollectionType ct) -> task<Result<bool>> {
    auto sql = Global::instance()->get_item_sql();
    auto ids = co_await sql->library_id_list();
    for (auto id : ids) {
        Action::instance()->sync_library_collection(id, ct);
    }
    co_return true;
}
auto SyncAPi::sync_collection(i64 library_id, enums::CollectionType ct) -> task<Result<bool>> {
    auto t      = QDateTime::currentDateTime();
    auto userId = Global::instance()->qsession()->user()->userId();
    if (auto c = Global::instance()->qsession()->client()) {
        auto out = co_await c->api->sync_collection(*c->instance, library_id, ct);
        Notifier::instance()->collection_synced(ct, userId, t);
        co_return out;
    }
    co_return false;
}
auto SyncAPi::sync_item(model::ItemId itemId, bool notify) -> task<Result<bool>> {
    if (auto c = Global::instance()->qsession()->client()) {
        auto out = co_await c->api->sync_items(*c->instance, std::array { itemId });
        if (notify) {
            Notifier::instance()->itemChanged(itemId);
        }
        co_return out;
    }
    co_return false;
}
auto SyncAPi::sync_items(std::span<const model::ItemId> itemId) -> task<Result<bool>> {
    if (auto c = Global::instance()->qsession()->client()) {
        co_return co_await c->api->sync_items(*c->instance, itemId);
    }
    co_return false;
}
auto SyncAPi::sync_list(enums::SyncListType type, model::ItemId itemId, i32 offset, i32 limit)
    -> task<Result<i32>> {
    if (auto c = Global::instance()->qsession()->client()) {
        co_return co_await c->api->sync_list(*c->instance, type, itemId, offset, limit);
    }
    co_return 0;
}
} // namespace qcm::query