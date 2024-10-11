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
        model::ItemId user_id;
        QString       type;
        model::ItemId item_id;
        QDateTime     created_at;
    };
    virtual auto get_executor() -> QtExecutor&                                               = 0;
    virtual auto insert(Item) -> asio::awaitable<void>                                       = 0;
    virtual auto select_id(model::ItemId user_id,
                           QString type = {}) -> asio::awaitable<std::vector<model::ItemId>> = 0;
};
} // namespace qcm::db