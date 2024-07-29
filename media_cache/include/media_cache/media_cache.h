#pragma once

#include <filesystem>

#include "core/core.h"
#include "media_cache/server.h"
#include "request/session.h"
#include "media_cache/database.h"

namespace media_cache
{

class MediaCache : NoCopy {
public:
    using executor_type = asio::thread_pool::executor_type;
    MediaCache(executor_type ex, rc<request::Session>);
    ~MediaCache();

    std::string get_url(std::string_view ori, std::string_view id) const;

    void start(std::filesystem::path cache_dir, rc<DataBase>);
    void stop();

private:
    rc<Server>            m_server;
    std::filesystem::path m_cache_dir;
};

} // namespace media_cache