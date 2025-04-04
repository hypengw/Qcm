#include "Qcm/query/artist_query.hpp"

#include "Qcm/backend.hpp"
#include "Qcm/app.h"

#include "qcm_interface/async.inl"

namespace qcm
{

ArtistListModel::ArtistListModel(QObject* parent): base_type(parent) {}

ArtistsQuery::ArtistsQuery(QObject* parent): query::QueryList<ArtistListModel>(parent) {
    // set_use_queue(true);
}
void ArtistsQuery::reload() {
    set_status(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::GetArtistsReq {};
    req.setPage(0);
    req.setPageSize((offset() + 1) * limit());
    auto self = helper::QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->inspect_set(rsp, [self](msg::GetArtistsRsp& el) {
            self->tdata()->resetModel(el.items());
            self->tdata()->setHasMore(el.hasMore());
        });
        co_return;
    });
}

void ArtistsQuery::fetchMore(qint32) {
    set_status(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::GetArtistsReq {};
    req.setPage(offset() + 1);
    req.setPageSize(limit());
    auto self = helper::QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto offset = req.page();
        auto rsp    = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->inspect_set(rsp, [self, offset](msg::GetArtistsRsp& el) {
            auto view = std::views::transform(el.items(), [](auto&& el) {
                return model::Artist(el);
            });
            self->tdata()->extend(view);
            self->setOffset(offset + 1);
            self->tdata()->setHasMore(el.hasMore());
        });
        co_return;
    });
}

} // namespace qcm

#include <Qcm/query/moc_artist_query.cpp>