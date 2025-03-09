#pragma once
#include "qcm_interface/global.h"

#include <mutex>
#include <QUuid>

#include "qcm_interface/plugin.h"
#include "qcm_interface/sql/item_sql.h"

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
    model::Session*        qsession;
    model::Session*        qsession_empty;
    model::Session*        loading_session;

    QUuid                     uuid;
    Arc<media_cache::DataBase> cache_sql;
    Arc<db::ColletionSqlBase>  collection_sql;
    Arc<db::ItemSqlBase>       item_sql;

    MetadataImpl metadata_impl;

    std::map<std::string, Client, std::less<>> clients;

    model::AppInfo   info;
    model::BusyInfo* busy_info;
    UserModel*       user_model;
    QQmlComponent*   copy_action_comp;
    state::AppState* app_state;

    mutable std::map<i64, model::Session*> provider_sessions;
    mutable std::set<i64>                  provider_ids;
    mutable std::map<i64, i64>             library_to_provider_id_map;
    mutable std::mutex                     mutex;
};
} // namespace qcm