#pragma once

#include "media_cache/database.h"
#include "core/core.h"

#include <QSqlDatabase>
#include <functional>
#include <asio/thread_pool.hpp>

namespace qcm
{

class CacheSql : public std::enable_shared_from_this<CacheSql>,
                 public media_cache::DataBase,
                 NoCopy {
public:
    using clean_cb_t = std::function<void(std::string_view)>;
    CacheSql(std::string_view table, i64 limit);
    ~CacheSql();

    auto& get_executor() { return m_ex; }

    void set_limit(i64);
    void set_clean_cb(clean_cb_t);

    asio::awaitable<std::optional<Item>> get(std::string key) override;
    asio::awaitable<void>                insert(Item) override;

    asio::awaitable<void>                remove(std::string key);
    asio::awaitable<usize>               total_size();
    asio::awaitable<std::optional<Item>> lru();
    asio::awaitable<std::vector<Item>>   get_all();

    asio::awaitable<void> try_clean();

private:
    void  try_connect();
    void  trigger_try_clean();
    usize total_size_sync();
    bool  create_table();
    bool  is_reached_limit();

    QString               m_table;
    asio::thread_pool     m_thread;
    asio::any_io_executor m_ex;
    QSqlDatabase          m_db;
    i64                   m_limit;
    double                m_total;
    bool                  m_connected;

    clean_cb_t m_clean_cb;
};

} // namespace qcm
