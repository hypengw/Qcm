#pragma once
#include <string>
#include <filesystem>
#include <asio/awaitable.hpp>
#include <QSize>

#include "qcm_interface/item_id.h"
#include "qcm_interface/enum.h"
#include "qcm_interface/router.h"
#include "asio_qt/qt_watcher.h"

#include "error/error.h"

namespace request
{
class Request;
}
namespace qcm
{
namespace model
{
class UserAccount;
class Session;
} // namespace model

struct ClientBase {};
struct Client {
    template<typename T>
    using Result = nstd::expected<T, error::Error>;

    struct ReqInfoImg {
        QSize size;
    };

    using ReqInfo = std::variant<ReqInfoImg>;

    struct Api {
        auto (*server_url)(ClientBase&, const model::ItemId&) -> std::string;
        bool (*make_request)(ClientBase&, request::Request&, const QUrl& url, const ReqInfo& info);
        void (*play_state)(ClientBase&, enums::PlaybackState state, model::ItemId item,
                           model::ItemId source, i64 played_second, QVariantMap extra);
        auto (*router)(ClientBase&) -> rc<Router>;

        auto (*logout)(ClientBase&) -> asio::awaitable<void>;
        auto (*session_check)(ClientBase&,
                              helper::QWatcher<model::Session>) -> asio::awaitable<Result<bool>>;
        auto (*collect)(ClientBase&, model::ItemId, bool) -> asio::awaitable<Result<bool>>;
        auto (*media_url)(ClientBase&, model::ItemId, enums::AudioQuality) -> asio::awaitable<Result<QUrl>>;

        void (*save)(ClientBase&, const std::filesystem::path&);
        void (*load)(ClientBase&, const std::filesystem::path&);

        std::string provider;
    };

    operator bool() const { return instance && api; }

    rc<Api>        api;
    rc<ClientBase> instance;
};

} // namespace qcm