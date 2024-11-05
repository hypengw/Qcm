#pragma once

#include <string>
#include <optional>

#include <asio/awaitable.hpp>
#include <QDateTime>

#include "qcm_interface/item_id.h"
#include "asio_qt/qt_executor.h"

namespace qcm::db
{

class ColletionSqlBase {
public:
    struct Item {
        model::ItemId            user_id;
        model::ItemId            item_id;
        QString                  type;
        std::optional<QDateTime> collect_time;

        static auto from(model::ItemId user_id, model::ItemId item_id) -> Item {
            Item out {};
            out.user_id = user_id;
            out.item_id = item_id;
            out.type    = item_id.type();
            return out;
        }
    };
    virtual ~ColletionSqlBase()                                         = default;
    virtual auto get_executor() -> QtExecutor&                          = 0;
    virtual auto insert(std::span<const Item>) -> asio::awaitable<bool> = 0;
    virtual auto remove(model::ItemId user_id, model::ItemId item_id) -> asio::awaitable<bool> = 0;
    virtual auto select_id(model::ItemId user_id,
                           QString type = {}) -> asio::awaitable<std::vector<model::ItemId>>   = 0;
    virtual auto refresh(model::ItemId user_id, QString type, std::span<const model::ItemId>,
                         std::span<const QDateTime> = {}) -> asio::awaitable<bool>             = 0;
};
} // namespace qcm::db