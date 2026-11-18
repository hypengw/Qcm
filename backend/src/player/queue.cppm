export module qcm.backend:player.queue;
export import rstd;
using rstd::collections::HashSet;
using rstd::random::Mt19937_64;
using namespace rstd::prelude;

export namespace qcm::backend
{
enum class LoopMode
{
    None,
    Track,
    Playlist
};

// Playback effects only; edits that keep the current entry still advance revision().
enum class PlayerQueueChange
{
    Unchanged,
    CurrentChanged,
    Restart,
    Exhausted
};

class PlayerQueue {
    Vec<i64>           song_ids_;
    Vec<u64>           entry_ids_;
    Vec<u64>           order_;
    Vec<u64>           remaining_;
    Vec<u64>           history_;
    usize              history_cursor_ {};
    i32                current_index_ { -1 };
    LoopMode           loop_mode_ { LoopMode::None };
    bool               shuffle_ {};
    bool               random_mode_ {};
    u64                revision_ {};
    u64                next_entry_id_ { 1 };
    Option<Mt19937_64> random_;

    static auto position(ref<u64[]> values, u64 value) -> usize;

    static auto signed_index(usize value) -> i32;

    static auto unsigned_index(i32 value) -> usize {
        return usize(static_cast<rstd::size_t>(value.to_primitive()));
    }

    template<typename T>
    static void insert_at(Vec<T>& target, usize index, ref<T[]> values);

    static void erase_entry(Vec<u64>& values, u64 entry);

    auto random_playback() const -> bool;

    auto next_revision() const -> Result<u64, String>;

    auto check_growth(usize count, usize retained) const -> Result<empty, String>;

    auto new_entries(usize count) const -> Vec<u64>;

    auto seed_random() -> Result<empty, String>;

    auto make_order(ref<u64[]> entries, Option<u64> first, bool shuffled)
        -> Result<Vec<u64>, String>;

    auto random_position(ref<u64[]> entries) -> Result<usize, String>;

    void reset_history();

    void trim_history();

    void remember(u64 entry);

    void prune_history(const HashSet<u64>& removed, bool current_removed);

    auto current_change(Option<u64> before) const -> PlayerQueueChange;

    auto advance_to(u64 entry) -> Result<PlayerQueueChange, String>;

public:
    PlayerQueue() = default;
    explicit PlayerQueue(u64 random_seed);

    auto ids() const [[clang::lifetimebound]] -> ref<i64[]> { return song_ids_.as_slice(); }
    auto entry_ids() const [[clang::lifetimebound]] -> ref<u64[]> { return entry_ids_.as_slice(); }
    auto current_index() const -> i32 { return current_index_; }
    auto current_id() const -> Option<i64> {
        if (current_index_ < i32()) return None();
        return Some<i64>(song_ids_[unsigned_index(current_index_)]);
    }
    auto current_entry_id() const -> Option<u64> {
        if (current_index_ < i32()) return None();
        return Some<u64>(entry_ids_[unsigned_index(current_index_)]);
    }
    auto revision() const -> u64 { return revision_; }
    auto loop_mode() const -> LoopMode { return loop_mode_; }
    auto shuffle() const -> bool { return shuffle_; }
    auto random_mode() const -> bool { return random_mode_; }

    auto can_next() const -> bool;
    auto can_next_without_loop() const -> bool;

    auto can_previous() const -> bool;

    auto replace(Vec<i64> songs, i32 index = i32(-1)) -> Result<PlayerQueueChange, String>;

    auto append(Vec<i64> songs) -> Result<PlayerQueueChange, String>;

    auto insert_next(Vec<i64> songs) -> Result<PlayerQueueChange, String>;

    auto clear() -> Result<PlayerQueueChange, String>;

    auto select(i32 index) -> Result<PlayerQueueChange, String>;

    auto remove(i32 index, i32 count = i32(1)) -> Result<PlayerQueueChange, String>;

    // `to` is the block's first index in the final queue, after removal from `from`.
    auto move(i32 from, i32 to, i32 count = i32(1)) -> Result<PlayerQueueChange, String>;

    auto next(bool ended = false) -> Result<PlayerQueueChange, String>;

    auto previous() -> Result<PlayerQueueChange, String>;

    auto set_loop_mode(LoopMode mode) -> Result<PlayerQueueChange, String>;

    auto set_shuffle(bool value) -> Result<PlayerQueueChange, String>;

    auto set_random_mode(bool value) -> Result<PlayerQueueChange, String>;
};
} // namespace qcm::backend
