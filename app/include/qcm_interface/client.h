#pragma once
#include <string>
#include <filesystem>
#include <QSize>

#include "core/asio/task.h"
#include "Qcm/model/item_id.hpp"
#include "Qcm/qml/enum.hpp"
#include "core/qasio/qt_watcher.h"

#include "error/error.h"

import ncrequest;

namespace qcm
{
namespace model
{
class UserAccount;
class Session;
} // namespace model

struct ClientBase : std::enable_shared_from_this<ClientBase>, NoCopy {
    std::string provider_name;
    i64         provider_id { -1 };
};

struct Client {
    template<typename T>
    using Result = nstd::expected<T, error::Error>;

    struct ReqInfoImg {
        QSize size;
    };

    using ReqInfo = std::variant<ReqInfoImg>;

    operator ClientBase&() const { return *instance; }

    struct Api {
        auto (*server_url)(ClientBase&, const model::ItemId&) -> std::string;
        bool (*make_request)(ClientBase&, ncrequest::Request&, const QUrl& url,
                             const ReqInfo& info);
        void (*play_state)(ClientBase&, enums::PlaybackState state, model::ItemId item,
                           model::ItemId source, i64 played_second, QVariantMap extra);

        auto (*logout)(ClientBase&) -> task<void>;
        auto (*session_check)(ClientBase&, helper::QWatcher<model::Session>) -> task<Result<bool>>;
        auto (*collect)(ClientBase&, model::ItemId, bool) -> task<Result<bool>>;
        auto (*media_url)(ClientBase&, model::ItemId, enums::AudioQuality) -> task<Result<QUrl>>;
        auto (*sync_library_list)(ClientBase&) -> task<Result<bool>>;
        auto (*sync_collection)(ClientBase&, i64 library_id, enums::CollectionType)
            -> task<Result<bool>>;
        auto (*sync_items)(ClientBase&, std::span<const model::ItemId>) -> task<Result<bool>>;
        auto (*sync_list)(ClientBase&, enums::SyncListType type, model::ItemId itemId, i32 offset,
                          i32 limit) -> task<Result<i32>>;

        auto (*create_mix)(ClientBase&, QString name) -> task<Result<model::ItemId>>;
        auto (*delete_mix)(ClientBase&, std::span<const model::ItemId> ids) -> task<Result<bool>>;
        auto (*rename_mix)(ClientBase&, model::ItemId id, QString name) -> task<Result<bool>>;
        auto (*manipulate_mix)(ClientBase&, model::ItemId id, enums::ManipulateMixAction act,
                               std::span<const model::ItemId> ids) -> task<Result<i32>>;

        void (*save)(ClientBase&, const std::filesystem::path&);
        void (*load)(ClientBase&, const std::filesystem::path&);

        std::string provider;
    };

    operator bool() const { return instance && api; }

    rc<Api>        api;
    rc<ClientBase> instance;
};

auto get_client() -> std::optional<Client>;

} // namespace qcm