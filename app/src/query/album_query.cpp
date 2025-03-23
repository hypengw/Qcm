#include "Qcm/query/album_query.hpp"

#include "Qcm/backend.h"
#include "Qcm/app.h"

#include "qcm_interface/async.inl"

namespace qcm
{

AlbumListModel::AlbumListModel(QObject* parent): base_type(parent), m_has_more(true) {}

auto AlbumListModel::hash(const value_type& i) const noexcept -> usize {
    return std::hash<QString> {}(i.id_proto());
}

AlbumsQuery::AlbumsQuery(QObject* parent): query::QueryList<AlbumListModel>(parent) {
    // set_use_queue(true);
}
void AlbumsQuery::reload() {
    set_status(Status::Querying);
    auto backend = App::instance()->backend();
    auto self    = helper::QWatcher { this };
    spawn([self, backend] -> task<void> {
        auto req = msg::GetAlbumsReq {};
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        // self->set(std::move(rsp));
        co_return;
    });
}
} // namespace qcm

#include <Qcm/query/moc_album_query.cpp>