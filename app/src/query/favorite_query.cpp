#include "Qcm/query/favorite_query.hpp"
#include "Qcm/util/async.inl"
#include "Qcm/app.hpp"
#include "Qcm/store.hpp"
#include "Qcm/backend.hpp"

namespace qcm
{

SetFavoriteQuery::SetFavoriteQuery(QObject* parent): Query(parent), m_favorite(false) {}

auto SetFavoriteQuery::favorite() const -> bool { return m_favorite; }
void SetFavoriteQuery::setFavorite(bool v) {
    if (m_favorite != v) {
        m_favorite = v;
        favoriteChanged();
    }
}
auto SetFavoriteQuery::itemId() const -> model::ItemId { return m_item_id; }

void SetFavoriteQuery::setItemId(model::ItemId id) {
    if (m_item_id != id) {
        m_item_id = id;
        itemIdChanged();
    }
}

void SetFavoriteQuery::reload() {
    if (! m_item_id.valid()) return;

    set_status(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::SetFavoriteReq {};
    req.setValue(m_favorite);
    req.setId_proto(m_item_id.id());
    req.setItemType(rstd::into(m_item_id.type()));

    auto self = helper::QWatcher { this };
    spawn([self, backend, req, value = m_favorite, item_id = m_item_id] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();

        self->inspect_set(rsp, [value, item_id](auto&) {
            if (auto ex = AppStore::instance()->extra(item_id)) {
                auto val = ex->value("dynamic").toMap();
                val.insert("is_favorite", value);
                ex->setProperty("dynamic", val);
            }
        });
        co_return;
    });
}

} // namespace qcm

#include "Qcm/query/moc_favorite_query.cpp"