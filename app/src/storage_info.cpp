#include "Qcm/storage_info.h"
#include "qcm_interface/async.inl"
#include "qcm_interface/global.h"
#include "Qcm/app.h"
#include "Qcm/sql/cache_sql.h"

#include "asio_helper/basic.h"

namespace qcm::qml
{

StorageInfo::StorageInfo(QObject* parent): QObject(parent) {}
auto StorageInfo::total() const -> double { return m_total; }
void StorageInfo::setTotal(double v) {
    if (ycore::cmp_exchange(m_total, v)) {
        totalChanged();
    }
}
StorageInfoQuerier::StorageInfoQuerier(QObject* parent): QAsyncResultT<StorageInfo>(parent) {}
void StorageInfoQuerier::reload() {
    set_status(Status::Querying);
    auto media_cache_sql = App::instance()->media_cache_sql();
    auto cache_sql       = App::instance()->cache_sql();
    this->spawn([media_cache_sql, cache_sql, this]() -> asio::awaitable<void> {
        auto media_size  = co_await media_cache_sql->total_size();
        auto normal_size = co_await cache_sql->total_size();

        co_await asio::post(
            asio::bind_executor(Global::instance()->qexecutor(), asio::use_awaitable));

        auto d = data().value<StorageInfo*>();
        d->setTotal(media_size + normal_size);
        set_status(Status::Finished);
    });
}
} // namespace qcm::qml