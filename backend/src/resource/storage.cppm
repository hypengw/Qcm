module;
#include <rstd/macro.hpp>
export module qcm.backend:resource.storage;
export import :resource.types;
export import :resource.sqlite;
export import :resource.schema;
export import :resource.segment_key;
import :resource.pack;
import :resource.startup;
import rstd.log;
using namespace rstd::prelude;
using namespace rstd::literals;
using rstd::tuple;
using rstd::bytes::Bytes;
using rstd::collections::HashMap;
using rstd::collections::HashSet;
using rstd::fs::File;
using rstd::path::Path;
using rstd::path::PathBuf;
using rstd::sync::Arc;
using rstd::sync::Weak;
using rstd::sync::atomic::Ordering;

export namespace qcm::backend::resource_cache
{
inline constexpr u64   CAPACITY { 32ULL * 1024 * 1024 * 1024 };
inline constexpr usize MAX_RESIDENT { 128 };
struct Resident {
    Metadata     meta;
    Index        index;
    Option<File> file;
    Weak<Lease>  lease;
    bool         plain {};
};
struct Build {
    i64 id;
    u64 remaining;
};
struct Ticket {
    Version version;
    File    file;
};
struct Segment {
    u64    number;
    String token;
    u64    length;
    bool   deleting {};
    auto   name(const Metadata&) const -> String;
    auto   header(const Metadata&) const -> DbResult<array<u8, 64>>;
};
auto valid_generation(ref<str>) -> bool;
auto token() -> DbResult<String>;
auto sync_directory(ref<Path>) -> DbResult<empty>;
auto regular_file(ref<Path>) -> DbResult<empty>;
auto private_directory(ref<Path>) -> DbResult<empty>;
auto private_file(const File&) -> DbResult<empty>;
auto allocated(ref<Path>) -> DbResult<u64>;
auto allocated(const File&) -> DbResult<u64>;
auto open_owner(ref<Path>) -> DbResult<File>;
auto managed_file_name(ref<str>) -> bool;
auto managed_segment_name(ref<str>) -> bool;
auto managed_pack_name(ref<str>) -> bool;
auto encode_metadata(const Metadata&) -> DbResult<String>;
auto encode_kind(ResourceKind) -> DbResult<i64>;
auto decode_kind(i64) -> DbResult<ResourceKind>;
auto decode_metadata(ref<str>) -> DbResult<Metadata>;
auto encode_object_metadata(const ObjectMetadata&) -> DbResult<String>;
auto decode_object_metadata(ref<str>) -> DbResult<ObjectMetadata>;

struct Storage {
    Database                                  db;
    PathBuf                                   root;
    HashMap<i64, Resident>                    versions;
    u64                                       capacity { CAPACITY };
    HashMap<SegmentKey, Weak<empty>>          pins;
    u64                                       segment_clock {};
    bool                                      stopped {};
    HashMap<String, tuple<u64, ResourceKind>> sizes;
    Option<Build>                             building;
    i64                                       scan_cursor {};
    u64                                       clock {};
    bool                                      clock_uncertain {};
    rstd::time::Instant                       started { rstd::time::Instant::now() };
    bool                                      files_enabled { true };
    Option<rstd::time::Instant>               restart_grace;
    HashSet<String>                           garbage;
    File                                      owner;
    Storage(PathBuf path, File lock): root(rstd::move(path)), owner(rstd::move(lock)) {}
    Storage(Storage&&) = default;
    static auto open(sqlito::Connection&, PathBuf, File, StartupTrace&) -> DbResult<Storage>;
    auto        path(ref<str>) const -> PathBuf;
    auto        recover(StartupTrace&) -> DbResult<empty>;
    auto        load(i64) -> DbResult<Resident*>;
    auto        version(i64) -> DbResult<Version>;
    auto        lookup(ref<str>) -> DbResult<Option<Version>>;
    auto        create(Metadata) -> DbResult<Version>;
    auto        create_layout(Metadata) -> DbResult<Version>;
    auto        read(i64, u64) -> DbResult<Option<Bytes>>;
    auto        write(i64, const Blocks&) -> DbResult<CommitOutcome>;
    auto        write_data(i64, const Blocks&) -> DbResult<empty>;
    auto        commit(i64) -> DbResult<empty>;
    auto        patch_index(i64, Index&) -> DbResult<empty>;
    auto        update(i64, Metadata) -> DbResult<empty>;
    auto        invalidate(i64) -> DbResult<empty>;
    auto        check_active(i64) -> DbResult<empty>;
    auto        reserve_index(usize) -> DbResult<empty>;
    auto        erase_version(i64) -> DbResult<empty>;
    auto        usage() -> DbResult<Usage>;
    auto        evict(u64) -> DbResult<empty>;
    auto        restore_segment_clock() -> DbResult<empty>;
    auto        segment_tick() -> u64;
    auto        pin_segments(i64, Range) -> Window;
    auto        segment(i64, u64) -> DbResult<Option<Segment>>;
    auto        segment_file(const Metadata&, const Segment&) -> DbResult<File>;
    auto        retain_segments(HashMap<String, ResourceKind>&) -> DbResult<empty>;
    auto        repair_segments(i64) -> DbResult<empty>;
    template<class F>
    auto publish_segment_index(i64 id, Index index, F operation) -> DbResult<empty> {
        rstd_try(db.atomic([&]() -> DbResult<empty> {
            rstd_try(patch_index(id, index));
            return operation();
        }));
        index.committed();
        auto value = versions.get_mut(id);
        if (value.is_none()) return Err(Error::state("version not resident"_str));
        (*value)->index = rstd::move(index);
        return Ok(empty {});
    }
    auto read_segment(i64, u64) -> DbResult<Option<Bytes>>;
    auto write_segments(i64, const Blocks&) -> DbResult<empty>;
    auto remove_segment(i64, u64) -> DbResult<empty>;
    auto delete_all_segments(i64) -> DbResult<empty>;
    auto evict_segments(u64) -> DbResult<empty>;
    auto copied_block(i64, u64) -> DbResult<empty>;
    auto file_path(ref<str>, ref<str>) const -> PathBuf;
    auto plain(i64) -> DbResult<bool>;
    auto plain_length(i64) -> DbResult<Option<u64>>;
    auto next_build() -> DbResult<Option<Ticket>>;
    auto check_build(i64) -> DbResult<empty>;
    auto publish_file(Version, array<u8, 32>) -> DbResult<empty>;
    auto defer_build(i64) -> DbResult<empty>;
    auto finish_failed(i64) -> DbResult<empty>;
    auto cleanup_plain_data() -> DbResult<empty>;
    auto remove_managed(ref<str>) -> DbResult<empty>;
    void cleanup_garbage();
    auto retire_corrupt(i64) -> DbResult<empty>;
    auto lease_now() const -> u64;
    auto restore_leases() -> DbResult<empty>;
    auto persist_clock() -> DbResult<empty>;
    auto expire_leases() -> DbResult<empty>;
    auto externally_pinned(i64) -> DbResult<bool>;
    auto acquire_file(i64, ref<str>) -> DbResult<Option<LocalFile>>;
    auto renew_file(ref<str>, ref<str>) -> DbResult<bool>;
    auto object(ref<str>) -> DbResult<Option<Object>>;
    auto put_object(ObjectMetadata, slice<u8>) -> DbResult<empty>;
    auto delete_pack(ref<str>) -> DbResult<empty>;
    auto retain_packs(HashMap<String, ResourceKind>&) -> DbResult<empty>;
    auto evict_packs(u64) -> DbResult<empty>;
    auto invalidate_packs(i64) -> DbResult<empty>;
};
} // namespace qcm::backend::resource_cache
