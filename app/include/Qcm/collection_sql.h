#pragma once

#include <QSqlDatabase>
#include <QDateTime>
#include <QThread>
#include <functional>

#include <asio/awaitable.hpp>

#include "core/core.h"
#include "qcm_interface/item_id.h"
#include "qcm_interface/collection_sql.h"

namespace qcm
{

class CollectionSql : public std::enable_shared_from_this<CollectionSql>,
                      public db::ColletionSqlBase,
                      NoCopy {
public:
    CollectionSql(std::string_view table);
    ~CollectionSql();

    auto get_executor() -> QtExecutor& override;
    auto insert(Item) -> asio::awaitable<void> override;
    auto select_id(model::ItemId user_id,
                   QString       type = {}) -> asio::awaitable<std::vector<model::ItemId>> override;

private:
    void connect_db();

    QThread                m_thread;
    rc<QtExecutionContext> m_ctx;
    QtExecutor             m_ex;
    QString                m_table;
    QSqlDatabase           m_db;
};

} // namespace qcm
