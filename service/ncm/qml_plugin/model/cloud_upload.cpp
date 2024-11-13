#include "service_qml_ncm/model/cloud_upload.h"

#include <asio/strand.hpp>
#include <asio/co_spawn.hpp>
#include <fstream>

#include "service_qml_ncm/api.h"
#include "service_qml_ncm/client.h"
#include "service_qml_ncm/model.h"
#include "qcm_interface/global.h"

#include "ncm/api/upload_cloud_info.h"
#include "ncm/api/cloud_upload_check.h"
#include "ncm/api/cloud_pub.h"
#include "ncm/api/nos_token_alloc.h"
#include "ncm/api/upload_addr.h"
#include "ncm/api/upload.h"

#include "crypto/crypto.h"
#include "core/log.h"

using namespace qcm;

namespace ncm::qml
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
    : qcm::QAsyncResultT<CloudUploadModel, NcmApiQueryBase>(parent) {}
CloudUploadApi::~CloudUploadApi() {}

void CloudUploadApi::reload() {}

auto CloudUploadApi::upload_impl(std::filesystem::path path)
    -> asio::awaitable<nstd::expected<std::monostate, error::Error>> {
    auto client = this->client();
    if (! client) co_return nstd::unexpected(error::Error::push("wrong session client"));

    ncm::api::CloudUploadCheck api;
    std::string                md5;
    usize                      size;
    {
        std::fstream f(path, std::ios_base::in | std::ios_base::binary);
        f.exceptions(std::ios_base::badbit);
        f.seekg(0, std::ios::end);
        size = f.tellg();
        f.seekg(0, std::ios::beg);
        auto res = crypto::digest(crypto::md5(), 1024 * 1024, [&f](std::span<byte> in) -> usize {
                       f.read((char*)in.data(), in.size());
                       auto s = f.gcount();
                       _assert_(s >= 0);
                       return std::max<std::streamsize>(s, 0);
                   }).map(crypto::hex::encode_low);
        if (! res) co_return UNEXPECTED(error::Error::push(""));
        md5 = convert_from<std::string>(res.value());
    }
    api.input.md5    = md5;
    api.input.length = size;
    ncm::api_model::CloudUploadCheck check_res;
    EC_RET_CO(check_res, co_await client->perform(api));

    if (true || check_res.needUpload) {
        ncm::api::NosTokenAlloc alloc_api;
        {
            auto& in  = alloc_api.input;
            in.bucket = ncm::params::NosTokenAlloc::bucket_private_cloud;
            in.ext    = path.extension();
            std::erase(in.ext, '.');
            in.filename = path.stem();
            in.md5      = md5;
        }
        ncm::api_model::NosTokenAlloc alloc_res;
        EC_RET_CO(alloc_res, co_await client->perform(alloc_api));

        ncm::api::UploadAddr addr_api;
        {
            auto& in  = addr_api.input;
            in.bucket = alloc_api.input.bucket;
        }
        ncm::api_model::UploadAddr addr_res;
        EC_RET_CO(addr_res, co_await client->perform(addr_api));

        if (! addr_res.upload.empty()) {
            auto meta = Global::instance()->get_metadata(path);

            std::fstream f(path, std::ios_base::in | std::ios_base::binary);
            f.exceptions(std::ios_base::badbit);
            ncm::api::Upload upload_api;
            {
                auto& in        = upload_api.input;
                in.upload_host  = addr_res.upload[0];
                in.bucket       = alloc_api.input.bucket;
                in.size         = size;
                in.content_md5  = md5;
                in.content_type = "audio/mpeg";
                if (! meta.streams.empty()) in.content_type = meta.streams.front().mime;
                in.nos_token = alloc_res.result.token;
                in.object    = alloc_res.result.objectKey;

                in.reader.set_size(size).set_callback([&f](byte* data, usize size) -> usize {
                    f.read((char*)data, size);
                    auto s = f.gcount();
                    _assert_(s >= 0);
                    return std::max<std::streamsize>(s, 0);
                });
            }
            ncm::api_model::Upload upload_res;
            EC_RET_CO(upload_res, co_await client->perform(upload_api, 180));

            ncm::api::UploadCloudInfo upload_info_api;
            {
                auto& in      = upload_info_api.input;
                in.songId     = check_res.songId;
                in.resourceId = std::to_string(alloc_res.result.resourceId);
                in.filename   = path.stem();
                in.song       = in.filename;
                auto set      = [&meta](std::string& in, const char* key) {
                    if (meta.tags.contains(key)) {
                        in = meta.tags.at(key);
                    }
                };
                set(in.song, "title");
                set(in.album, "album");
                set(in.artist, "artist");

                if (! meta.streams.empty()) {
                    in.bitrate = meta.streams.front().bitrate;
                }
                in.md5 = md5;
            }
            ncm::api_model::UploadCloudInfo upload_info_res;
            EC_RET_CO(upload_info_res, co_await client->perform(upload_info_api));

            ncm::api::CloudPub pub_api;
            {
                pub_api.input.songId = upload_info_res.songId;
            }
            ncm::api_model::CloudPub pub_res;
            EC_RET_CO(pub_res, co_await client->perform(pub_api));
        }
    }
    co_return std::monostate {};
}

void CloudUploadApi::upload(const QUrl& file) {
    DEBUG_LOG("{}", file.toLocalFile());
    auto ex     = asio::strand<Global::pool_executor_t> { Global::instance()->pool_executor() };
    auto api_ex = get_executor();
    auto self   = QPointer<CloudUploadApi>(this);
    // auto                     alloc = asio::recycling_allocator<void>();
    asio::co_spawn(
        ex,
        [self, api_ex, file]() mutable -> asio::awaitable<void> {
            auto res = co_await self->upload_impl(file.toLocalFile().toStdString());

            co_await asio::post(asio::bind_executor(api_ex, asio::use_awaitable));
            if (self) {
                if (! res) {
                    DEBUG_LOG("{}", res.error());
                    self->set_error(convert_from<QString>(res.error().what()));
                    self->set_status(Status::Error);
                } else {
                    self->set_status(Status::Finished);
                }
            }
            co_return;
        },
        [self, api_ex](std::exception_ptr ptr) {
            if (! ptr) return;
            asio::post(api_ex, [self, ptr]() {
                if (self) {
                    try {
                        std::rethrow_exception(ptr);
                    } catch (const std::exception& e) {
                        self->set_error(convert_from<QString>(e.what()));
                        self->set_status(Status::Error);
                    }
                }
            });
        });
}

} // namespace ncm::qml