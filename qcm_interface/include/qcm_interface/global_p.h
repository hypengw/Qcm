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

    qt_executor_t     qt_ex;
    asio::thread_pool pool;

    rc<request::Session> session;
    model::Session*      qsession;
    model::Session*      qsession_empty;
    model::Session*      loading_session;

    QUuid                     uuid;
    rc<media_cache::DataBase> cache_sql;
    rc<db::ColletionSqlBase>  collection_sql;
    rc<db::ItemSqlBase>      album_sql;

    MetadataImpl metadata_impl;

    std::map<std::string, Client, std::less<>> clients;

    model::AppInfo   info;
    Action*          action;
    model::BusyInfo* busy_info;
    UserModel*       user_model;
    QQmlComponent*   copy_action_comp;
    state::AppState* app_state;

    std::mutex mutex;
};
} // namespace qcm