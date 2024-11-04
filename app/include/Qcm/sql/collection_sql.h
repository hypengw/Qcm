#pragma once

#include <asio/awaitable.hpp>

#include "core/core.h"
#include "asio_helper/task.h"
#include "qcm_interface/item_id.h"
#include "qcm_interface/sql/collection_sql.h"

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
    auto con() const -> rc<helper::SqlConnect>;
    auto insert(std::span<const Item>) -> task<bool> override;
    auto remove(model::ItemId user_id, model::ItemId item_id) -> task<bool> override;
    auto select_id(model::ItemId user_id,
                   QString       type = {}) -> task<std::vector<model::ItemId>> override;

    auto refresh(model::ItemId user_id, QString type, std::span<const model::ItemId>,
                 std::span<const QDateTime> = {}) -> task<bool> override;

    bool insert_sync(std::span<const Item>);
    bool insert_sync(model::ItemId user_id, std::span<const model::ItemId>, std::span<const QDateTime> = {});
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
