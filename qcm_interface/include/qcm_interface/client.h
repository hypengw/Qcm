#pragma once
#include <string>
#include <filesystem>
#include <asio/awaitable.hpp>

#include "qcm_interface/item_id.h"
#include "qcm_interface/enum.h"
#include "qcm_interface/router.h"
#include "asio_qt/qt_watcher.h"

#include "error/error.h"

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
    struct Api {
        auto (*server_url)(ClientBase&, const model::ItemId&) -> std::string;
        auto (*image_cache)(ClientBase&, const QUrl& url, QSize req) -> std::filesystem::path;
        void (*play_state)(ClientBase&, enums::PlaybackState state, model::ItemId item,
                           model::ItemId source, i64 played_second, QVariantMap extra);
        auto (*router)(ClientBase&) -> rc<Router>;
        auto (*logout)(ClientBase&) -> asio::awaitable<void>;
        auto (*session_check)(ClientBase&,
                              helper::QWatcher<model::Session>) -> asio::awaitable<Result<bool>>;
        void (*save)(ClientBase&, const std::filesystem::path&);
        void (*load)(ClientBase&, const std::filesystem::path&);

        std::string provider;
    };

    operator bool() const { return instance && api; }

    rc<Api>        api;
    rc<ClientBase> instance;
};

} // namespace qcm