#pragma once

#include <asio/awaitable.hpp>

#include <set>
#include "core/core.h"
#include "core/asio/task.h"
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
    auto remove(model::ItemId user_id, std::span<const model::ItemId> ids) -> task<bool>;
    auto select_id(model::ItemId user_id,
                   QString       type = {}) -> task<std::vector<model::ItemId>> override;

    auto select_removed(model::ItemId user_id, const QString& type,
                        const QDateTime& time) -> task<std::vector<model::ItemId>>;
    auto select_missing(const model::ItemId& user_id, std::string_view type, std::string_view join,
                        const std::set<std::string>& not_null) -> task<std::vector<model::ItemId>>;
    auto select_missing(const model::ItemId& user_id, std::string_view type, std::string_view join,
                        const QMetaObject& meta) -> task<std::vector<model::ItemId>>;

    auto refresh(model::ItemId user_id, i64 provider_id, QString type, std::span<const model::ItemId>,
                 std::span<const QDateTime> = {}) -> task<bool> override;

    bool insert_sync(std::span<const Item>);
    bool insert_sync(model::ItemId user_id, std::span<const model::ItemId>,
                     std::span<const QDateTime> = {});
    bool remove_sync(model::ItemId user_id, std::span<const model::ItemId> ids);
    bool remove_sync(model::ItemId user_id, i64 provider_id, QString type = {});
    bool delete_with(model::ItemId user_id, i64 provider_id, QString type = {});
    bool delete_removed();

private:
    void connect_db();

    QString                m_table;
    rc<helper::SqlConnect> m_con;
};

} // namespace qcm
