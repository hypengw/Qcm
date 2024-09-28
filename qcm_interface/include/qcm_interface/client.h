#pragma once
#include <string>
#include <filesystem>
#include "qcm_interface/item_id.h"
#include "qcm_interface/enum.h"
#include "qcm_interface/router.h"

#include <asio/awaitable.hpp>

namespace qcm
{
namespace model
{
class UserAccount;
}

struct ClientBase {};
struct Client {
    struct Api {
        auto (*server_url)(ClientBase&, const model::ItemId&) -> std::string;
        auto (*image_cache)(ClientBase&, const QUrl& url, QSize req) -> std::filesystem::path;
        void (*play_state)(ClientBase&, enums::PlaybackState state, model::ItemId item,
                           model::ItemId source, i64 played_second, QVariantMap extra);
        auto (*router)(ClientBase&) -> rc<Router>;
        void (*user_check)(ClientBase&, model::UserAccount*);
        auto (*logout)(ClientBase&) -> asio::awaitable<void>;
    };

    operator bool() const { return instance && api; }

    rc<Api>        api;
    rc<ClientBase> instance;
};

} // namespace qcm