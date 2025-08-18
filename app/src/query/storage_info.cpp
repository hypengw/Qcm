#include "Qcm/query/storage_info.hpp"
#include "Qcm/util/async.inl"
#include "Qcm/global.hpp"
#include "Qcm/app.hpp"
#include "Qcm/backend.hpp"

#include "core/asio/basic.h"

namespace qcm::qml
{

StorageInfo::StorageInfo(QObject* parent)
    : QObject(parent), m_total(0), m_media(0), m_image(0), m_database(0) {}
auto StorageInfo::total() const -> double { return m_total; }
void StorageInfo::setTotal(double v) {
    if (ycore::cmp_set(m_total, v)) {
        totalChanged();
    }
}
auto StorageInfo::image() const -> double { return m_image; }
void StorageInfo::setImage(double v) {
    if (ycore::cmp_set(m_image, v)) {
        imageChanged();
    }
}
auto StorageInfo::media() const -> double { return m_media; }
void StorageInfo::setMedia(double v) {
    if (ycore::cmp_set(m_media, v)) {
        mediaChanged();
    }
}
auto StorageInfo::database() const -> double { return m_database; }
void StorageInfo::setDatabase(double v) {
    if (ycore::cmp_set(m_database, v)) {
        databaseChanged();
    }
}
void StorageInfo::updateTotal() { setTotal(m_media + m_image + m_database); }

StorageInfoQuery::StorageInfoQuery(QObject* parent): Query(parent) {}
void StorageInfoQuery::reload() {
    setStatus(Status::Querying);

    auto backend = App::instance()->backend();
    auto self    = helper::QWatcher { this };
    this->spawn([backend, self]() -> task<void> {
        auto req = msg::GetStorageInfoReq {};
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->inspect_set(rsp, [self](msg::GetStorageInfoRsp& el) {
            auto t = self->tdata();
            t->setImage(el.imageSize());
            t->setMedia(el.mediaSize());
            t->setDatabase(el.databaseSize());
            t->updateTotal();
        });
    });
}
} // namespace qcm::qml

#include <Qcm/query/moc_storage_info.cpp>