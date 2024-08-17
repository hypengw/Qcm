#include "service_qml_ncm/api.h"

#include <QUuid>
#include "qcm_interface/global.h"
#include "service_qml_ncm/model.h"
#include "service_qml_ncm/ncm_image.h"

namespace ncm::impl
{

static auto server_url(std::any& client, const qcm::model::ItemId& id) -> std::string {
    auto c = std::any_cast<ncm::Client>(&client);
    switch (ncm_id_type(id)) {
    case ncm::model::IdType::Song: {
        return fmt::format("{}/#song?id={}", ncm::BASE_URL, id.id());
    }
    default: {
        return {};
    }
    }
}
static auto image_cache(std::any& client, const QUrl& url, QSize reqSize) -> std::filesystem::path {
    auto c = std::any_cast<ncm::Client>(&client);
    return qcm::NcmImageProvider::genImageCachePath(
        qcm::NcmImageProvider::makeReq(url.toString(), reqSize, *c));
}

} // namespace ncm::impl

namespace qcm
{

ncm::Client detail::get_client() {
    auto a = Global::instance()->client("ncm", []() -> Global::Client {
        auto api         = make_rc<Global::Client::Api>();
        api->server_url  = ncm::impl::server_url;
        api->image_cache = ncm::impl::image_cache;

        return { .api = api,
                 .instance =
                     ncm::Client(Global::instance()->session(),
                                 Global::instance()->pool_executor(),
                                 ncm::api::device_id_from_uuid(
                                     Global::instance()->uuid().toString().toStdString())) };
    });
    return std::any_cast<ncm::Client>(a.instance);
}
} // namespace qcm