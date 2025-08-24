#pragma once

#include <mutex>
#include <QUuid>

#include "Qcm/global.hpp"
#include "Qcm/player.hpp"

namespace qcm
{

auto get_pool_size() -> std::size_t;

class Global::Private {
public:
    Private(Global* p);
    ~Private();

    Arc<QtExecutionContext> qt_ctx;
    asio::thread_pool       pool;

    Arc<ncrequest::Session> session;

    QUuid uuid;

    MetadataImpl metadata_impl;

    Player* player;

    mutable std::mutex mutex;
};
} // namespace qcm