#pragma once
#include "qcm_interface/global.h"

#include <mutex>
#include <QUuid>

namespace qcm
{

auto get_pool_size() -> std::size_t;

class Global::Private {
public:
    Private(Global* p);
    ~Private();

    Arc<QtExecutionContext> qt_ctx;
    asio::thread_pool  pool;

    Arc<ncrequest::Session> session;

    QUuid                     uuid;
    Arc<media_cache::DataBase> cache_sql;

    MetadataImpl metadata_impl;

    std::map<std::string, Client, std::less<>> clients;

    model::AppInfo   info;
    QQmlComponent*   copy_action_comp;
    state::AppState* app_state;

    mutable std::mutex                     mutex;
};
} // namespace qcm