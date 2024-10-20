#pragma once

#include <asio/awaitable.hpp>

#include "core/core.h"
#include "qcm_interface/item_id.h"
#include "qcm_interface/collection_sql.h"

namespace helper
{
class SqlConnect;
}
namespace qcm
{

class CollectionSql : public db::ColletionSqlBase, NoCopy {
public:
    CollectionSql(std::string_view table, rc<helper::SqlConnect> con);
    ~CollectionSql();

    auto get_executor() -> QtExecutor& override;
    auto insert(std::span<const Item>) -> asio::awaitable<bool> override;
    auto remove(model::ItemId user_id, model::ItemId item_id) -> asio::awaitable<bool> override;
    auto select_id(model::ItemId user_id,
                   QString       type = {}) -> asio::awaitable<std::vector<model::ItemId>> override;

    auto refresh(model::ItemId user_id, QString type,
                 std::span<const model::ItemId>) -> asio::awaitable<bool> override;

    bool insert_sync(std::span<const Item>);
    bool insert_sync(model::ItemId userId, std::span<const model::ItemId>);
    bool remove_sync(model::ItemId user_id, model::ItemId item_id);
    bool delete_with(model::ItemId user_id, QString type = {});
    bool un_valid(model::ItemId user_id, QString type = {});
    bool clean_not_valid();

private:
    void connect_db();

    QString                m_table;
    rc<helper::SqlConnect> m_con;
};

} // namespace qcm
