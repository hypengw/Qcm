module;
#include <rstd/macro.hpp>
module qcm.backend;
import :resource.storage;
import rstd.log;
using namespace rstd::prelude;
using namespace rstd::literals;
using rstd::time::Duration;
using rstd::time::Instant;

namespace qcm::backend::resource_cache
{
void Storage::defer_remove(String name) {
    if (! sizes.contains_key(name) || garbage.contains_key(name)) return;
    if (! managed_file_name(name.as_str()) && ! managed_segment_name(name.as_str()) &&
        ! managed_pack_name(name.as_str()))
        return;
    auto after = Instant::now() + Duration::from_millis(u64(50));
    garbage.insert(rstd::move(name), PendingRemoval { after });
    cleanup_at = None();
}
auto Storage::cleanup_deadline() -> Option<Instant> {
    if (cleanup_at.is_none())
        for (auto entry : garbage.iter()) {
            auto after = rstd::get<1>(entry)->after;
            if (cleanup_at.is_none() || after < *cleanup_at) cleanup_at = Some(after);
        }
    return cleanup_at;
}
auto Storage::remove_managed(ref<str> name, Instant now) -> DbResult<empty> {
    if (! managed_file_name(name) && ! managed_segment_name(name) && ! managed_pack_name(name))
        return Err(Error::state("unmanaged cache path"_str));
    if (auto pending = garbage.get(name);
        pending.is_some() && (*pending)->retry_seconds != u64() && now < (*pending)->after)
        return Err(Error::state("cache file removal awaiting retry"_str));
    auto result = [&]() -> DbResult<empty> {
        auto removed = rstd::fs::remove_file(root.join(name).as_path());
        if (removed.is_err()) {
            auto cause = rstd::move(removed).unwrap_err();
            if (cause.kind().code != rstd::io::ErrorKind::NotFound)
                return Err(Error::io(rstd::move(cause)));
        }
        return sync_directory(root.as_path());
    }();
    if (result.is_err()) {
        u64 seconds(1);
        if (auto pending = garbage.get_mut(name)) {
            seconds =
                rstd::cmp::min(u64(30), rstd::cmp::max(u64(1), (*pending)->retry_seconds * u64(2)));
            **pending = PendingRemoval { now + Duration::from_secs(seconds), seconds };
        } else {
            garbage.insert(String::make(name),
                           PendingRemoval { now + Duration::from_secs(seconds), seconds });
        }
        cleanup_at = None();
        return result;
    }
    garbage.remove(name);
    cleanup_at = None();
    sizes.remove(name);
    return Ok(empty {});
}
void Storage::cleanup_garbage(Instant now, bool urgent) {
    if (garbage.is_empty() || (! urgent && now < *cleanup_deadline())) return;
    Vec<String> names;
    for (auto entry : garbage.iter()) {
        const auto& pending = *rstd::get<1>(entry);
        if (pending.after > now && (! urgent || pending.retry_seconds != u64())) continue;
        names.push(rstd::get<0>(entry)->clone());
        if (names.len() == usize(64)) break;
    }
    for (const auto& name : names)
        if (auto cleanup = remove_managed(name.as_str(), now); cleanup.is_err())
            rstd::log::debug("cache cleanup still deferred: {}", rstd::move(cleanup).unwrap_err());
}
} // namespace qcm::backend::resource_cache
