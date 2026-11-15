export module qcm.backend:resource.startup;
import qcm.backend.startup_trace;
import rstd;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend::resource_cache
{
enum class StartupStage
{
    Total,
    Files,
    Storage,
    Migrate,
    SegmentClock,
    Leases,
    Recovery,
    Versions,
    RetainedLeases,
    InvalidVersions,
    Segments,
    Packs,
    Directory,
    Eviction,
    Count
};
inline constexpr ref<str> STARTUP_LABELS[] = {
    "resource.storage.total"_str,
    "resource.files.open"_str,
    "resource.storage.open"_str,
    "resource.database.migrate"_str,
    "resource.segment_clock.restore"_str,
    "resource.leases.restore"_str,
    "resource.recover"_str,
    "resource.recover.versions"_str,
    "resource.recover.leases"_str,
    "resource.recover.invalid"_str,
    "resource.recover.segments"_str,
    "resource.recover.packs"_str,
    "resource.recover.directory"_str,
    "resource.recover.evict"_str,
};
static_assert(sizeof(STARTUP_LABELS) / sizeof(STARTUP_LABELS[0]) ==
              static_cast<rstd::size_t>(StartupStage::Count));
using StartupTrace = startup::Trace<StartupStage>;
} // namespace qcm::backend::resource_cache
