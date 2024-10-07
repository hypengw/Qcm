#include "service_qml_ncm/client.h"

#include <QUuid>
#include <asio/bind_executor.hpp>

#include "qcm_interface/async.inl"
#include "qcm_interface/global.h"
#include "qcm_interface/model/session.h"

#include "service_qml_ncm/model.h"
#include "service_qml_ncm/ncm_image_p.h"
#include "asio_helper/detached_log.h"
#include "asio_qt/qt_holder.h"

#include "ncm/api/feedback_weblog.h"
#include "ncm/api/user_account.h"
#include "ncm/api/logout.h"
#include "ncm/client.h"

namespace ncm::impl
{
using ClientBase = qcm::ClientBase;

struct Client : public ClientBase {
    Client(ncm::Client ncm): ncm(ncm) {}
    ncm::Client ncm;
};

static auto get_client(ClientBase& c) -> ncm::Client* { return &static_cast<Client&>(c).ncm; }

static auto server_url(ClientBase& cbase, const qcm::model::ItemId& id) -> std::string {
    auto c = get_client(cbase);
    switch (UNWRAP(ncm_id_type(id))) {
    case ncm::model::IdType::Song: {
        return fmt::format("{}/#song?id={}", ncm::BASE_URL, id.id());
    }
    default: {
        return {};
    }
    }
}
static auto image_cache(ClientBase& cbase, const QUrl& url,
                        QSize reqSize) -> std::filesystem::path {
    auto c = get_client(cbase);
    return qcm::NcmImageProvider::genImageCachePath(
        qcm::NcmImageProvider::makeReq(url.toString(), reqSize, *c));
}

static void play_state(ClientBase& cbase, qcm::enums::PlaybackState state,
                       qcm::model::ItemId itemId, qcm::model::ItemId sourceId, i64 played_second,
                       QVariantMap) {
    if (state == qcm::enums::PlaybackState::PausedState) return;
    auto c         = *get_client(cbase);
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

static auto router(ClientBase& cbase) -> rc<qcm::Router> {
    auto c          = *get_client(cbase);
    auto router_any = c.prop("router"sv);
    if (! router_any) {
        c.set_prop("router"sv, make_rc<qcm::Router>());
        router_any = c.prop("router"sv);
    }
    auto router = std::any_cast<rc<qcm::Router>>(router_any);
    return router;
}

static auto logout(ClientBase& cbase) -> asio::awaitable<void> {
    auto             c = *get_client(cbase);
    ncm::api::Logout api;
    co_await c.perform(api);
    co_return;
}
static auto session_check(ClientBase& cbase, helper::QWatcher<qcm::model::Session> session)
    -> asio::awaitable<Result<bool>> {
    auto                  c = *get_client(cbase);
    ncm::api::UserAccount api;
    auto                  out = co_await c.perform(api);
    co_await asio::post(
        asio::bind_executor(qcm::Global::instance()->qexecutor(), asio::use_awaitable));

    auto user = session->user();
    co_return out.transform([user](const auto& out) -> bool {
        if (out.profile) {
            user->set_userId(convert_from<ItemId>(out.profile->userId));
            user->set_nickname(convert_from<QString>(out.profile->nickname));
            user->set_avatarUrl(convert_from<QString>(out.profile->avatarUrl));

            return true;
        }
        return false;
    });
}
void save(ClientBase& cbase, const std::filesystem::path& path) {
    auto c = *get_client(cbase);
    c.save(path);
}
void load(ClientBase& cbase, const std::filesystem::path& path) {
    auto c = *get_client(cbase);
    c.load(path);
}

} // namespace ncm::impl

namespace ncm::qml
{

auto create_client() -> qcm::Client {
    auto c = ncm::Client(
        qcm::Global::instance()->session(),
        qcm::Global::instance()->pool_executor(),
        ncm::api::device_id_from_uuid(qcm::Global::instance()->uuid().toString().toStdString()));

    auto api      = make_rc<qcm::Global::Client::Api>();
    auto instance = make_rc<ncm::impl::Client>(c);

    api->provider      = ncm::qml::provider;
    api->server_url    = ncm::impl::server_url;
    api->image_cache   = ncm::impl::image_cache;
    api->play_state    = ncm::impl::play_state;
    api->router        = ncm::impl::router;
    api->logout        = ncm::impl::logout;
    api->session_check = ncm::impl::session_check;
    api->save          = ncm::impl::save;
    api->load          = ncm::impl::load;

    return { .api = api, .instance = instance };
}

auto get_ncm_client(qcm::Client& c) -> ncm::Client& {
    return static_cast<ncm::impl::Client&>(*c.instance).ncm;
}

} // namespace ncm::qml