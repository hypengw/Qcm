#pragma once

#include <asio/awaitable.hpp>

#include "core/core.h"
#include "qcm_interface/sql/item_sql.h"

namespace helper
{
class SqlConnect;
}
namespace qcm
{

class ItemSql : public db::ItemSqlBase {
public:
    ItemSql(rc<helper::SqlConnect> con);
    ~ItemSql();

    auto get_executor() -> QtExecutor& override;
    auto con() const -> rc<helper::SqlConnect>;

    auto insert(std::span<const model::Album> items,
                const std::set<std::string>&  on_update) -> asio::awaitable<bool> override;

    auto insert(std::span<const model::Artist> items,
                const std::set<std::string>&   on_update) -> asio::awaitable<bool> override;

    auto insert_album_artist(std::span<const std::tuple<model::ItemId, model::ItemId>>) -> asio::awaitable<bool> override;

private:
    void create_album_table();
    void create_artist_table();
    void create_album_artist_table();

    QString                m_album_table;
    QString                m_artist_table;
    QString                m_album_artist_table;
    rc<helper::SqlConnect> m_con;
};

} // namespace qcm
