#include "service_qml_ncm/api.h"

#include <QUuid>
#include <asio/bind_executor.hpp>
#include "qcm_interface/global.h"
#include "service_qml_ncm/model.h"
#include "service_qml_ncm/ncm_image_p.h"
#include "asio_helper/detached_log.h"
#include "asio_qt/qt_holder.h"

#include "ncm/api/feedback_weblog.h"
#include "ncm/api/user_account.h"

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
    auto c         = *std::any_cast<ncm::Client>(&client);
    auto state_old = c.prop("play_state");
    if (state == qcm::enums::PlaybackState::PlayingState) {
        if (state_old) {
            if (std::any_cast<qcm::enums::PlaybackState>(state_old.value()) == state) {
                return;
            }
        }
    }
    c.set_prop("play_state", state);

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

static auto router(std::any& client) -> rc<qcm::Router> {
    auto c          = *std::any_cast<ncm::Client>(&client);
    auto router_any = c.prop("router"sv);
    if (! router_any) {
        c.set_prop("router"sv, make_rc<qcm::Router>());
        router_any = c.prop("router"sv);
    }
    auto router = std::any_cast<rc<qcm::Router>>(router_any);
    return router;
}

static void user_check(std::any& client, qcm::model::UserAccount* user) {
    auto c  = *std::any_cast<ncm::Client>(&client);
    auto ex = asio::make_strand(c.get_executor());

    ncm::api::UserAccount api;
    auto                  res = qcm::Global::instance()->user_model()->check_result();

    res->spawn(
        ex, [c, api, res, hold = helper::create_qholder(user)]() mutable -> asio::awaitable<void> {
            auto out = co_await c.perform(api);
            co_await asio::post(asio::bind_executor(res->get_executor(), asio::use_awaitable));
            res->from(out.transform([user = hold->data()](const ncm::api_model::UserAccount& in) {
                user->set_userId(convert_from<ItemId>(in.profile->userId));
                user->set_nickname(convert_from<QString>(in.profile->nickname));
                user->set_avatarUrl(convert_from<QString>(in.profile->avatarUrl));
                return user;
            }));
            co_return;
        });
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
        api->router      = ncm::impl::router;
        api->user_check  = ncm::impl::user_check;

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