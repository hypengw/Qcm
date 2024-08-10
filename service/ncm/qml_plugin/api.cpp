#include "service_qml_ncm/api.h"

#include "qcm_interface/global.h"
#include "service_qml_ncm/model.h"

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

} // namespace ncm::impl

namespace qcm
{

ncm::Client detail::get_client() {
    auto a = Global::instance()->client("ncm", []() -> Global::Client {
        auto api        = make_rc<Global::Client::Api>();
        api->server_url = ncm::impl::server_url;
        return { .api      = api,
                 .instance = ncm::Client(Global::instance()->session(),
                                         Global::instance()->pool_executor()) };
    });
    return std::any_cast<ncm::Client>(a.instance);
}
} // namespace qcm