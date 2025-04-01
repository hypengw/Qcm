#include "Qcm/query/album_query.hpp"

#include "Qcm/backend.hpp"
#include "Qcm/app.h"
#include "Qcm/model/app_store.hpp"

#include "qcm_interface/async.inl"

namespace qcm
{

AlbumListModel::AlbumListModel(QObject* parent): base_type(parent) {}
QQmlPropertyMap* AlbumListModel::extra(i32 idx) const {
    if (auto extend = App::instance()->app_store()->albums.query_extend(this->key_at(idx));
        extend) {
        return extend->extra.get();
    }
    return nullptr;
}

AlbumsQuery::AlbumsQuery(QObject* parent): query::QueryList<AlbumListModel>(parent) {
    // set_use_queue(true);
    this->tdata()->set_store(App::instance()->app_store()->albums);
}

static const std::set<QStringView> json_fields { u"artists" };

void AlbumsQuery::reload() {
    set_status(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::GetAlbumsReq {};
    req.setPage(0);
    req.setPageSize((offset() + 1) * limit());
    auto self = helper::QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->inspect_set(rsp, [self](msg::GetAlbumsRsp& el) {
            self->tdata()->resetModel(el.items());
            for (qsizetype i = 0; i < el.extras().size(); i++) {
                auto id = el.items().at(i).id_proto().toLongLong();
                if (auto extend = App::instance()->app_store()->albums.query_extend(id); extend) {
                    msg::merge_extra(*(extend->extra), el.extras().at(i), json_fields);
                }
            }
            self->tdata()->setHasMore(el.hasMore());
        });
        co_return;
    });
}

void AlbumsQuery::fetchMore(qint32) {
    set_status(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::GetAlbumsReq {};
    req.setPage(offset() + 1);
    req.setPageSize(limit());
    auto self = helper::QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto offset = req.page();
        auto rsp    = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->inspect_set(rsp, [self, offset](msg::GetAlbumsRsp& el) {
            self->tdata()->extend(el.items());

            for (qsizetype i = 0; i < el.extras().size(); i++) {
                auto id = el.items().at(i).id_proto().toLongLong();
                if (auto extend = App::instance()->app_store()->albums.query_extend(id); extend) {
                    msg::merge_extra(*(extend->extra), el.extras().at(i), json_fields);
                }
            }
            self->setOffset(offset + 1);
            self->tdata()->setHasMore(el.hasMore());
        });
        co_return;
    });
}

} // namespace qcm

#include <Qcm/query/moc_album_query.cpp>