#pragma once
#include <any>
#include <string>
#include <filesystem>
#include "qcm_interface/item_id.h"
#include "qcm_interface/enum.h"
#include "qcm_interface/router.h"

namespace qcm
{
namespace model
{
class UserAccount;
}
struct Client {
    struct Api {
        auto (*server_url)(std::any&, const model::ItemId&) -> std::string;
        auto (*image_cache)(std::any&, const QUrl& url, QSize req) -> std::filesystem::path;
        void (*play_state)(std::any&, enums::PlaybackState state, model::ItemId item,
                           model::ItemId source, i64 played_second, QVariantMap extra);
        auto (*router)(std::any&) -> rc<Router>;
        void (*user_check)(std::any&, model::UserAccount*);
    };

    operator bool() const { return instance.has_value(); }

    rc<Api>  api;
    std::any instance;
};

} // namespace qcm