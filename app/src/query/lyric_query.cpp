#include "Qcm/query/lyric_query.hpp"
#include "Qcm/app.hpp"
#include "Qcm/backend.hpp"
#include "Qcm/util/async.inl"

namespace qcm
{
LyricQuery::LyricQuery(QObject* parent): Query(parent) {
    connect_requet_reload(&LyricQuery::itemIdChanged);
}
void LyricQuery::reload() {
    setStatus(Status::Querying);
    auto app     = App::instance();
    auto backend = app->backend();
    auto req     = msg::GetSubtitleReq {};

    if (m_item_id.valid()) {
        req.setSongId(m_item_id.id());

        auto self = helper::QWatcher { this };
        spawn([self, backend, req] mutable -> task<void> {
            auto rsp = co_await backend->send(std::move(req));
            co_await qcm::qexecutor_switch();
            auto t = self->tdata();
            if (rsp) {
                msg::GetSubtitleRsp& el = *rsp;
                auto view = std::views::transform(el.subtitle().items(), [](auto& el) {
                    return LyricItem { .milliseconds = el.start(), .content = el.text() };
                });
                t->setCurrentIndex(-1);
                t->resetModel(view);
                t->setCurrentIndex(-1);
            } else {
                t->resetModel();
                t->setCurrentIndex(-1);
            }
            self->setStatus(Status::Finished);
            co_return;
        });
    } else {
        cancel();
        auto t = this->tdata();
        t->resetModel();
        t->setCurrentIndex(-1);
        setStatus(Status::Finished);
    }
}

auto LyricQuery::itemId() const -> model::ItemId { return m_item_id; }
void LyricQuery::setItemId(model::ItemId id) {
    if (ycore::cmp_set(m_item_id, id)) {
        itemIdChanged();
    }
}
} // namespace qcm

#include "Qcm/query/moc_lyric_query.cpp"