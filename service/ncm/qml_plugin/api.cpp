#include "service_qml_ncm/api.h"

#include <QUuid>
#include "qcm_interface/global.h"
#include "service_qml_ncm/model.h"
#include "service_qml_ncm/ncm_image_p.h"
#include "ncm/api/feedback_weblog.h"
#include "asio_helper/detached_log.h"

namespace ncm::impl
{

static auto server_url(std::any& client, const qcm::model::ItemId& id) -> std::string {
    auto c = std::any_cast<ncm::Client>(&client);
    switch (UNWRAP(ncm_id_type(id))) {
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

static void play_state(std::any& client, qcm::enums::PlaybackState state, qcm::model::ItemId itemId,
                       qcm::model::ItemId sourceId, i64 played_second, QVariantMap) {
    if (state == qcm::enums::PlaybackState::PausedState) return;

    auto                     c  = *std::any_cast<ncm::Client>(&client);
    auto                     ex = asio::make_strand(c.get_executor());
    ncm::api::FeedbackWeblog api;

    if (! helper::variant_convert(api.input.id, to_ncm_id(itemId))) {
        ERROR_LOG("");
        return;
    }

    api.input.act  = state == qcm::enums::PlaybackState::PlayingState
                         ? ncm::params::FeedbackWeblog::Action::Start
                         : ncm::params::FeedbackWeblog::Action::End;
    api.input.time = played_second;

    helper::variant_convert(api.input.sourceId, to_ncm_id(sourceId));

    asio::co_spawn(
        ex,
        [c, api]() mutable -> asio::awaitable<void> {
            co_await c.perform(api);
            co_return;
        },
        helper::asio_detached_log);
}

} // namespace ncm::impl

namespace qcm
{

ncm::Client detail::get_client() {
    auto a = Global::instance()->client("ncm", []() -> Global::Client {
        auto api         = make_rc<Global::Client::Api>();
        api->server_url  = ncm::impl::server_url;
        api->image_cache = ncm::impl::image_cache;
        api->play_state  = ncm::impl::play_state;

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