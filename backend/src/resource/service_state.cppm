module;
#include <rstd/enum.hpp>
module qcm.backend:resource.service_state;
import :resource.service;
import :resource.store;
import :resource.download_key;
using namespace rstd::prelude;
using rstd::async::Notify;
using rstd::async::Semaphore;
namespace watch = rstd::async::watch;
using rstd::bytes::Bytes;
using rstd::collections::HashMap;
using rstd::sync::Arc;
using rstd::sync::Weak;

namespace qcm::backend::resource_cache
{
struct ResourceCache::ReaderPosition {
    rstd::sync::atomic::Atomic<u64> cursor;
    explicit ReaderPosition(u64 n): cursor(n) {}
};
class ResourceCache::Progress {
    RSTD_ENUM(Progress, (Running), (Complete), (Failed, (Error error;)))
public:
    auto clone() const -> Progress;
};
struct ResourceCache::DownloadState {
    HashMap<u64, Bytes>       blocks;
    Vec<Weak<ReaderPosition>> readers;
};
struct ResourceCache::Download {
    i64                              provider;
    i64                              version;
    Range                            range;
    watch::Sender<bool>              cancel { false };
    watch::Sender<Progress>          progress { Progress::Running() };
    rstd::sync::Mutex<DownloadState> state;
    Notify                           readable;
    Download(i64 provider, i64 version, Range range, Notify readable);
};
struct ResourceCache::BlockOutput {
    Bytes                 bytes;
    Option<Arc<Download>> download;
};

struct ResourceCache::State {
    Weak<ResourceCache> self_ { Weak<ResourceCache>::make() };
    Option<Store>       store;
    rstd::async::Mutex<HashMap<String, Weak<rstd::async::Mutex<empty>>>> opens;
    rstd::async::Mutex<HashMap<DownloadKey, Weak<Download>>>             downloads;
    Semaphore                                                            readers { usize(16) };
    Semaphore                                                            buffers { usize(8) };
    Semaphore                                                            requests { usize(4) };
    rstd::async::Mutex<HashMap<i64, Weak<Semaphore>>>                    per_resource;
    watch::Sender<bool>                                                  stop { false };
    rstd::async::Mutex<Vec<rstd::async::JoinHandle<empty>>>              tasks;
    rstd::sync::Mutex<Option<String>>                                    file_client;

    explicit State(Option<Store> store);
};
} // namespace qcm::backend::resource_cache
