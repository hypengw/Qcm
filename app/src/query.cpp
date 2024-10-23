#include "Qcm/query/album_collection.h"
#include "Qcm/query/artist_collection.h"

namespace qcm::query
{
auto SyncAPi::sync_collection(enums::CollectionType ct) -> task<void> {
    if (auto c = Global::instance()->qsession()->client()) {
        co_await c->api->sync_collection(*c->instance, ct);
    }
    co_return;
}
auto SyncAPi::sync_item(model::ItemId itemId) -> task<void> {
    if (auto c = Global::instance()->qsession()->client()) {
        co_await c->api->sync_item(*c->instance, itemId);
    }
    co_return;
}
} // namespace qcm::query