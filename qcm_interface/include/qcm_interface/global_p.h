#pragma once
#include "qcm_interface/global.h"

#include <mutex>
#include <QUuid>

#include "qcm_interface/plugin.h"

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
    //    mutable ncm::Client         m_client;

    QUuid                     uuid;
    rc<media_cache::DataBase> cache_sql;

    MetadataImpl metadata_impl;

    std::map<std::string, Client, std::less<>> clients;

    model::AppInfo info;
    QQmlComponent* copy_action_comp;

    std::map<QString, QcmPluginInterface*> plugins;

    std::mutex mutex;
};
} // namespace qcm