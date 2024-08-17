#include "service_qml_ncm/model/cloud_upload.h"

#include <asio/strand.hpp>
#include <asio/co_spawn.hpp>

#include "ncm/api/cloud_upload_check.h"
#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "qcm_interface/global.h"

#include "core/log.h"

namespace qcm::qml_ncm
{

auto CloudUploadModel::rowCount(const QModelIndex&) const -> int { return m_items.size(); }
auto CloudUploadModel::data(const QModelIndex& index, int role) const -> QVariant {
    auto row = index.row();
    if (row < rowCount()) {
        switch (role) {
        case Qt::UserRole: {
        }
        }
    }
    return {};
}
auto CloudUploadModel::roleNames() const -> QHash<int, QByteArray> {
    return { { Qt::UserRole, "id" } };
}

auto CloudUploadApi::data() const -> QObject* { return m_data; }
void CloudUploadApi::reload() {}

auto CloudUploadApi::upload_impl() -> asio::awaitable<void> {
    auto                       client = detail::get_client();
    ncm::api::CloudUploadCheck api;
    api.input.songId = "9023840";
    auto res         = co_await client.perform(api);
    co_return;
}

void CloudUploadApi::upload(const QUrl& file) {
    DEBUG_LOG("{}", file.toLocalFile());
    auto ex = asio::strand<Global::pool_executor_t> { Global::instance()->pool_executor() };
    // auto                     alloc = asio::recycling_allocator<void>();
    asio::co_spawn(
        ex,
        [this]() mutable -> asio::awaitable<void> {
            co_await this->upload_impl();
            co_return;
        },
        [](std::exception_ptr) {
        });
}

} // namespace qcm::qml_ncm