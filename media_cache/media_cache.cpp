#include "media_cache/media_cache.h"

#include "core/log.h"

#include "request/type.h"

using namespace media_cache;

MediaCache::MediaCache(asio::any_io_executor ex, rc<request::Session> s)
    : m_server(std::make_shared<Server>(ex, s)) {}
MediaCache::~MediaCache() { stop(); }

void MediaCache::start(std::filesystem::path cache_dir, rc<DataBase> db) {
    m_server->start(cache_dir, db);
}

void MediaCache::stop() { m_server->stop(); }

std::string MediaCache::get_url(std::string_view ori, std::string_view id) const {
    request::UrlParams p;
    p.set_param("url", ori);
    return fmt::format("http://127.0.0.1:{}/{}?{}", m_server->port(), id, p.encode());
}
