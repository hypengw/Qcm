#include "service_qml_ncm/model/cloud_upload.h"

#include <asio/strand.hpp>
#include <asio/co_spawn.hpp>
#include <fstream>

#include "ncm/api/cloud_upload_check.h"
#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "qcm_interface/global.h"

#include "crypto/crypto.h"
#include "core/log.h"

namespace qcm::qml_ncm
{

CloudUploadModel::CloudUploadModel(QObject* parent): QAbstractListModel(parent) {}
CloudUploadModel::~CloudUploadModel() {}

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

CloudUploadApi::CloudUploadApi(QObject* parent)
    : ApiQuerierBase(parent), m_data(new CloudUploadModel(this)) {}
CloudUploadApi::~CloudUploadApi() {}

auto CloudUploadApi::data() const -> QObject* { return m_data; }
void CloudUploadApi::reload() {}

auto CloudUploadApi::upload_impl(std::filesystem::path path) -> asio::awaitable<void> {
    auto                       client = detail::get_client();
    ncm::api::CloudUploadCheck api;
    std::fstream               f(path, std::ios_base::in | std::ios_base::binary);
    auto md5 = crypto::digest(crypto::md5(), 1024 * 1024, [&f](std::span<byte> in) -> usize {
                   f.read((char*)in.data(), in.size());
                   return f.gcount();
               }).map(crypto::hex::encode_low);
    if (! md5) co_return;

    api.input.md5 = convert_from<std::string>(md5.value());
    auto res      = co_await client.perform(api);
    co_return;
}

void CloudUploadApi::upload(const QUrl& file) {
    DEBUG_LOG("{}", file.toLocalFile());
    auto ex = asio::strand<Global::pool_executor_t> { Global::instance()->pool_executor() };
    // auto                     alloc = asio::recycling_allocator<void>();
    asio::co_spawn(
        ex,
        [this, file]() mutable -> asio::awaitable<void> {
            co_await this->upload_impl(file.toLocalFile().toStdString());
            co_return;
        },
        [](std::exception_ptr) {
        });
}

} // namespace qcm::qml_ncm