#pragma once

#include "media_cache/database.h"
#include "core/core.h"
#include "core/asio/task.h"

#include <functional>
#include "asio_qt/qt_executor.h"

namespace helper
{
class SqlConnect;
}
namespace qcm
{

class CacheSql : public std::enable_shared_from_this<CacheSql>,
                 public media_cache::DataBase,
                 NoCopy {
public:
    using clean_cb_t = std::function<void(std::string_view)>;
    CacheSql(std::string_view table, i64 limit, rc<helper::SqlConnect> db);
    ~CacheSql();

    auto get_executor() -> asio::any_io_executor override;
    auto get_qexecutor() -> QtExecutor&;

    void set_limit(i64);
    void set_clean_cb(clean_cb_t);

    task<std::optional<Item>> get(std::string key) override;
    task<void>                insert(Item) override;

    task<void>                remove(std::string key);
    task<void>                remove(std::span<std::string> key);
    task<usize>               total_size();
    task<std::optional<Item>> lru();
    task<std::vector<Item>>   get_all();

    task<void> try_clean();

private:
    void  try_connect();
    void  trigger_try_clean();
    usize total_size_sync();
    bool  create_table();
    bool  is_reached_limit();

    rc<helper::SqlConnect> m_con;
    QString                m_table;
    i64                    m_limit;
    double                 m_total;
    clean_cb_t             m_clean_cb;
};

} // namespace qcm
