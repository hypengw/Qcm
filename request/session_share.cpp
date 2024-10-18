#include "request/session_share.h"
#include "curl_easy.h"
#include <mutex>

namespace request
{

class SessionShare::Private {
public:
    Private(): share(curl_share_init()) {}
    ~Private() { curl_share_cleanup(share); }

    CURLSH*    share;
    std::mutex share_mutex;
};

namespace
{
static void static_share_lock(CURL*, curl_lock_data data, curl_lock_access, void* clientp) {
    auto info = static_cast<SessionShare::Private*>(clientp);
    if (data == curl_lock_data::CURL_LOCK_DATA_COOKIE) {
        info->share_mutex.lock();
    }
}

static void static_share_unlock(CURL*, curl_lock_data data, void* clientp) {
    auto info = static_cast<SessionShare::Private*>(clientp);
    if (data == curl_lock_data::CURL_LOCK_DATA_COOKIE) {
        info->share_mutex.unlock();
    }
}
} // namespace

SessionShare::SessionShare(): d_ptr(make_rc<Private>()) {
    C_D(SessionShare);
    curl_share_setopt(d->share, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
    curl_share_setopt(d->share, CURLSHOPT_LOCKFUNC, static_share_lock);
    curl_share_setopt(d->share, CURLSHOPT_UNLOCKFUNC, static_share_unlock);
    curl_share_setopt(d->share, CURLSHOPT_USERDATA, d);
}
SessionShare::~SessionShare() {}

auto SessionShare::handle() const -> voidp {
    C_D(const SessionShare);
    return d->share;
}
void SessionShare::load(const std::filesystem::path& p) {
    C_D(SessionShare);
    CurlEasy x;
    x.setopt(CURLOPT_SHARE, d->share);
    // append filename
    x.setopt(CURLOPT_COOKIEFILE, p.c_str());
    // actually load
    x.setopt(CURLOPT_COOKIELIST, "RELOAD");
}

void SessionShare::save(const std::filesystem::path& p) const {
    C_D(const SessionShare);
    CurlEasy x;
    x.setopt(CURLOPT_SHARE, d->share);
    x.setopt(CURLOPT_COOKIEJAR, p.c_str());
    // save when x destruct
}

} // namespace request