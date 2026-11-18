module;
#include <rstd/macro.hpp>
module qcm.backend;
import :player.queue;
using rstd::collections::HashSet;
using rstd::random::Mt19937_64;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend
{
auto PlayerQueue::position(ref<u64[]> values, u64 value) -> usize {
    for (usize index {}; index < values.len(); ++index)
        if (values[index] == value) return index;
    return values.len();
}

auto PlayerQueue::signed_index(usize value) -> i32 {
    return i32(static_cast<rstd::int32_t>(value.to_primitive()));
}

template<typename T>
void PlayerQueue::insert_at(Vec<T>& target, usize index, ref<T[]> values) {
    auto result = Vec<T>::with_capacity(target.len() + values.len());
    for (usize i {}; i < index; ++i) result.push_back(target[i]);
    result.extend_from_slice(values);
    for (auto i = index; i < target.len(); ++i) result.push_back(target[i]);
    target = rstd::move(result);
}

void PlayerQueue::erase_entry(Vec<u64>& values, u64 entry) {
    values.retain([&](u64 value) {
        return value != entry;
    });
}

auto PlayerQueue::random_playback() const -> bool { return shuffle_ && random_mode_; }

auto PlayerQueue::next_revision() const -> Result<u64, String> {
    auto next = revision_.checked_add(u64(1));
    if (next.is_none()) return Err("player queue revision exhausted"_Str);
    return Ok(*next);
}

auto PlayerQueue::check_growth(usize count, usize retained) const -> Result<empty, String> {
    auto limit = usize(static_cast<rstd::size_t>(i32::MAX.to_primitive()));
    if (count > limit - retained) return Err("player queue exceeds index range"_Str);
    if (u64(count.to_primitive()) > u64::MAX - next_entry_id_)
        return Err("player queue entry ids exhausted"_Str);
    return Ok(empty {});
}

auto PlayerQueue::new_entries(usize count) const -> Vec<u64> {
    auto entries = Vec<u64>::with_capacity(count);
    for (usize i {}; i < count; ++i) entries.push(next_entry_id_ + u64(i.to_primitive()));
    return entries;
}

auto PlayerQueue::seed_random() -> Result<empty, String> {
    if (random_.is_some()) return Ok(empty {});
    auto seed = rstd::random::random(rstd::random::Standard<u64> {});
    if (seed.is_err())
        return Err(rstd::format("player queue random seed: {}", seed.as_ref().unwrap_err()));
    random_ = Some(Mt19937_64::from_seed(*seed));
    return Ok(empty {});
}

auto PlayerQueue::make_order(ref<u64[]> entries, Option<u64> first, bool shuffled)
    -> Result<Vec<u64>, String> {
    Vec<u64> result;
    for (auto entry : entries)
        if (first.is_none() || entry != *first) result.push_back(entry);
    if (shuffled && result.len() > usize(1)) {
        rstd_try(seed_random());
        rstd::random::shuffle(result.as_mut_slice().as_mut_ref(), *random_).unwrap();
    }
    if (first.is_some()) result.insert(usize(), rstd::move(*first));
    return Ok(rstd::move(result));
}

auto PlayerQueue::random_position(ref<u64[]> entries) -> Result<usize, String> {
    if (entries.is_empty()) return Err("player queue has no random candidates"_Str);
    auto current  = current_entry_id();
    auto excluded = current.is_some() ? position(entries, *current) : entries.len();
    bool skip     = entries.len() > usize(1) && excluded < entries.len();
    auto count    = entries.len() - usize(skip ? 1 : 0);
    if (count == usize(1)) return Ok(skip && excluded == usize() ? usize(1) : usize());
    rstd_try(seed_random());
    auto range = rstd::random::UniformInt<usize>::make(usize(), count - usize(1)).unwrap();
    auto index = rstd::random::sample(range, *random_).unwrap();
    if (skip && index >= excluded) ++index;
    return Ok(index);
}

void PlayerQueue::reset_history() {
    history_.clear();
    history_cursor_ = usize();
    remaining_.clear();
    if (! random_playback()) return;
    remaining_   = entry_ids_.clone();
    auto current = current_entry_id();
    if (current.is_some()) {
        history_.push_back(*current);
        erase_entry(remaining_, *current);
    }
}

void PlayerQueue::trim_history() {
    if (entry_ids_.is_empty()) {
        history_.clear();
        history_cursor_ = usize();
        return;
    }
    if (history_.len() <= entry_ids_.len()) return;
    // Keep a bounded window containing the current visit, including after queue shrinkage.
    auto start = history_cursor_ + usize(1) > entry_ids_.len()
                     ? history_cursor_ + usize(1) - entry_ids_.len()
                     : usize();
    auto count = entry_ids_.len();
    for (usize i {}; i < count; ++i) history_[i] = history_[start + i];
    history_.truncate(count);
    history_cursor_ -= start;
}

void PlayerQueue::remember(u64 entry) {
    if (! history_.is_empty()) history_.truncate(history_cursor_ + usize(1));
    if (history_.is_empty() || *history_.last().unwrap() != entry) history_.push_back(entry);
    history_cursor_ = history_.len() - usize(1);
    trim_history();
}

void PlayerQueue::prune_history(const HashSet<u64>& removed, bool current_removed) {
    if (history_.is_empty()) return;
    auto  length = current_removed ? history_cursor_ + usize(1) : history_.len();
    usize written {}, cursor {};
    for (usize read {}; read < length; ++read) {
        auto entry = history_[read];
        if (removed.contains(entry)) continue;
        if (written == usize() || history_[written - usize(1)] != entry)
            history_[written++] = entry;
        if (read <= history_cursor_) cursor = written - usize(1);
    }
    history_.truncate(written);
    history_cursor_ = cursor;
    trim_history();
}

auto PlayerQueue::current_change(Option<u64> before) const -> PlayerQueueChange {
    return before == current_entry_id() ? PlayerQueueChange::Unchanged
                                        : PlayerQueueChange::CurrentChanged;
}

auto PlayerQueue::advance_to(u64 entry) -> Result<PlayerQueueChange, String> {
    if (current_entry_id() == Some(entry)) return Ok(PlayerQueueChange::Restart);
    auto revision  = rstd_try(next_revision());
    current_index_ = signed_index(position(entry_ids_.as_slice(), entry));
    revision_      = revision;
    return Ok(PlayerQueueChange::CurrentChanged);
}

PlayerQueue::PlayerQueue(u64 random_seed): random_(Some(Mt19937_64::from_seed(random_seed))) {}

auto PlayerQueue::can_next() const -> bool {
    return can_next_without_loop() ||
           (! entry_ids_.is_empty() && loop_mode_ == LoopMode::Playlist);
}

auto PlayerQueue::can_next_without_loop() const -> bool {
    if (entry_ids_.is_empty()) return false;
    if (current_index_ < i32()) return true;
    if (random_playback())
        return history_cursor_ + usize(1) < history_.len() || ! remaining_.is_empty();
    auto at = position(order_.as_slice(), *current_entry_id());
    return at + usize(1) < order_.len();
}

auto PlayerQueue::can_previous() const -> bool {
    if (current_index_ < i32()) return false;
    if (random_playback()) return history_cursor_ > usize();
    return position(order_.as_slice(), *current_entry_id()) > usize() ||
           loop_mode_ == LoopMode::Playlist;
}

auto PlayerQueue::replace(Vec<i64> songs, i32 index) -> Result<PlayerQueueChange, String> {
    rstd_try(check_growth(songs.len(), usize()));
    if (index < i32(-1) || (index >= i32() && unsigned_index(index) >= songs.len()))
        return Err("player queue replacement index out of range"_Str);
    if (songs.is_empty() && song_ids_.is_empty()) return Ok(PlayerQueueChange::Unchanged);
    auto        revision = rstd_try(next_revision());
    auto        before   = current_entry_id();
    auto        entries  = new_entries(songs.len());
    Option<u64> first;
    if (index >= i32()) first = Some(entries[unsigned_index(index)]);
    auto order =
        shuffle_ ? rstd_try(make_order(entries.as_slice(), first, true)) : entries.clone();
    next_entry_id_ += u64(entries.len().to_primitive());
    song_ids_      = rstd::move(songs);
    entry_ids_     = rstd::move(entries);
    order_         = rstd::move(order);
    current_index_ = index;
    reset_history();
    revision_ = revision;
    return Ok(current_change(before));
}

auto PlayerQueue::append(Vec<i64> songs) -> Result<PlayerQueueChange, String> {
    rstd_try(check_growth(songs.len(), song_ids_.len()));
    if (songs.is_empty()) return Ok(PlayerQueueChange::Unchanged);
    auto revision = rstd_try(next_revision());
    auto entries  = new_entries(songs.len());
    auto order    = rstd_try(make_order(entries.as_slice(), None(), shuffle_));
    next_entry_id_ += u64(entries.len().to_primitive());
    song_ids_.append(songs);
    entry_ids_.extend_from_slice(entries.as_slice());
    order_.append(order);
    if (random_playback()) remaining_.append(entries);
    revision_ = revision;
    return Ok(PlayerQueueChange::Unchanged);
}

auto PlayerQueue::insert_next(Vec<i64> songs) -> Result<PlayerQueueChange, String> {
    rstd_try(check_growth(songs.len(), song_ids_.len()));
    if (songs.is_empty()) return Ok(PlayerQueueChange::Unchanged);
    auto revision = rstd_try(next_revision());
    auto entries  = new_entries(songs.len());
    auto index = current_index_ < i32() ? usize() : unsigned_index(current_index_) + usize(1);
    auto order_index = current_index_ < i32()
                           ? usize()
                           : position(order_.as_slice(), *current_entry_id()) + usize(1);
    insert_at(song_ids_, index, songs.as_slice());
    insert_at(entry_ids_, index, entries.as_slice());
    insert_at(order_, order_index, entries.as_slice());
    if (random_playback()) {
        auto following = current_index_ < i32() ? usize() : history_cursor_ + usize(1);
        insert_at(history_, following, entries.as_slice());
        remaining_.extend_from_slice(entries.as_slice());
    }
    next_entry_id_ += u64(entries.len().to_primitive());
    revision_ = revision;
    return Ok(PlayerQueueChange::Unchanged);
}

auto PlayerQueue::clear() -> Result<PlayerQueueChange, String> {
    if (song_ids_.is_empty()) return Ok(PlayerQueueChange::Unchanged);
    auto revision = rstd_try(next_revision());
    auto before   = current_entry_id();
    song_ids_.clear();
    entry_ids_.clear();
    order_.clear();
    current_index_ = i32(-1);
    reset_history();
    revision_ = revision;
    return Ok(current_change(before));
}

auto PlayerQueue::select(i32 index) -> Result<PlayerQueueChange, String> {
    if (index < i32() || unsigned_index(index) >= song_ids_.len())
        return Err("player queue selection index out of range"_Str);
    if (index == current_index_) return Ok(PlayerQueueChange::Unchanged);
    auto revision = rstd_try(next_revision());
    if (current_index_ < i32()) history_.clear();
    current_index_ = index;
    if (random_playback()) {
        auto entry = *current_entry_id();
        remember(entry);
        erase_entry(remaining_, entry);
    }
    revision_ = revision;
    return Ok(PlayerQueueChange::CurrentChanged);
}

auto PlayerQueue::remove(i32 index, i32 count) -> Result<PlayerQueueChange, String> {
    if (index < i32() || count < i32() || unsigned_index(index) > song_ids_.len() ||
        unsigned_index(count) > song_ids_.len() - unsigned_index(index))
        return Err("player queue removal range out of bounds"_Str);
    if (count == i32()) return Ok(PlayerQueueChange::Unchanged);
    auto         revision = rstd_try(next_revision());
    auto         before   = current_entry_id();
    auto         start    = unsigned_index(index);
    auto         amount   = unsigned_index(count);
    HashSet<u64> removed;
    for (auto i = start; i < start + amount; ++i) removed.insert(entry_ids_[i]);
    bool current_removed = before.is_some() && removed.contains(*before);
    for (auto read = start + amount; read < song_ids_.len(); ++read) {
        song_ids_[read - amount]  = song_ids_[read];
        entry_ids_[read - amount] = entry_ids_[read];
    }
    song_ids_.truncate(song_ids_.len() - amount);
    entry_ids_.truncate(entry_ids_.len() - amount);
    order_.retain([&](u64 entry) {
        return ! removed.contains(entry);
    });
    remaining_.retain([&](u64 entry) {
        return ! removed.contains(entry);
    });
    prune_history(removed, current_removed);
    if (entry_ids_.is_empty()) {
        current_index_ = i32(-1);
        reset_history();
    } else if (current_removed) {
        current_index_ =
            signed_index(start < entry_ids_.len() ? start : entry_ids_.len() - usize(1));
        if (random_playback()) {
            auto entry = *current_entry_id();
            remember(entry);
            erase_entry(remaining_, entry);
        }
    } else if (before.is_some()) {
        current_index_ = signed_index(position(entry_ids_.as_slice(), *before));
    }
    revision_ = revision;
    return Ok(current_change(before));
}

auto PlayerQueue::move(i32 from, i32 to, i32 count) -> Result<PlayerQueueChange, String> {
    if (from < i32() || to < i32() || count < i32() || unsigned_index(from) > song_ids_.len() ||
        unsigned_index(count) > song_ids_.len() - unsigned_index(from) ||
        unsigned_index(to) > song_ids_.len() - unsigned_index(count))
        return Err("player queue move range out of bounds"_Str);
    if (count == i32() || from == to) return Ok(PlayerQueueChange::Unchanged);
    auto     revision = rstd_try(next_revision());
    auto     before   = current_entry_id();
    auto     start = unsigned_index(from), destination = unsigned_index(to);
    auto     amount = unsigned_index(count);
    Vec<i64> songs;
    Vec<u64> entries;
    songs.resize(song_ids_.len(), i64());
    entries.resize(entry_ids_.len(), u64());
    for (usize i {}; i < song_ids_.len(); ++i) {
        auto target = i;
        if (i >= start && i < start + amount)
            target = destination + i - start;
        else if (start < destination && i >= start + amount && i < destination + amount)
            target = i - amount;
        else if (destination < start && i >= destination && i < start)
            target = i + amount;
        songs[target]   = song_ids_[i];
        entries[target] = entry_ids_[i];
    }
    song_ids_  = rstd::move(songs);
    entry_ids_ = rstd::move(entries);
    if (! shuffle_) order_ = entry_ids_.clone();
    if (before.is_some())
        current_index_ = signed_index(position(entry_ids_.as_slice(), *before));
    revision_ = revision;
    return Ok(PlayerQueueChange::Unchanged);
}

auto PlayerQueue::next(bool ended) -> Result<PlayerQueueChange, String> {
    if (entry_ids_.is_empty()) return Ok(PlayerQueueChange::Exhausted);
    if (ended && current_index_ >= i32() && loop_mode_ == LoopMode::Track)
        return Ok(PlayerQueueChange::Restart);
    if (! random_playback()) {
        if (current_index_ < i32()) return advance_to(order_[usize()]);
        auto at = position(order_.as_slice(), *current_entry_id());
        if (at + usize(1) < order_.len()) return advance_to(order_[at + usize(1)]);
        if (loop_mode_ == LoopMode::Playlist) return advance_to(order_[usize()]);
        return Ok(PlayerQueueChange::Exhausted);
    }
    if (! history_.is_empty() &&
        (current_index_ < i32() || history_cursor_ + usize(1) < history_.len())) {
        auto revision   = rstd_try(next_revision());
        auto cursor     = current_index_ < i32() ? usize() : history_cursor_ + usize(1);
        auto entry      = history_[cursor];
        auto before     = current_entry_id();
        current_index_  = signed_index(position(entry_ids_.as_slice(), entry));
        history_cursor_ = cursor;
        erase_entry(remaining_, entry);
        revision_ = revision;
        return Ok(current_change(before) == PlayerQueueChange::Unchanged
                      ? PlayerQueueChange::Restart
                      : PlayerQueueChange::CurrentChanged);
    }
    Vec<u64> refill;
    if (remaining_.is_empty()) {
        if (loop_mode_ != LoopMode::Playlist) return Ok(PlayerQueueChange::Exhausted);
        refill = entry_ids_.clone();
    }
    auto candidates = remaining_.is_empty() ? refill.as_slice() : remaining_.as_slice();
    auto selected   = rstd_try(random_position(candidates));
    auto entry      = candidates[selected];
    if (current_entry_id() == Some(entry)) return Ok(PlayerQueueChange::Restart);
    auto revision = rstd_try(next_revision());
    if (! refill.is_empty()) remaining_ = rstd::move(refill);
    erase_entry(remaining_, entry);
    current_index_ = signed_index(position(entry_ids_.as_slice(), entry));
    remember(entry);
    revision_ = revision;
    return Ok(PlayerQueueChange::CurrentChanged);
}

auto PlayerQueue::previous() -> Result<PlayerQueueChange, String> {
    if (! can_previous()) return Ok(PlayerQueueChange::Unchanged);
    if (random_playback()) {
        auto revision = rstd_try(next_revision());
        --history_cursor_;
        auto before = current_entry_id();
        current_index_ =
            signed_index(position(entry_ids_.as_slice(), history_[history_cursor_]));
        revision_ = revision;
        return Ok(current_change(before) == PlayerQueueChange::Unchanged
                      ? PlayerQueueChange::Restart
                      : PlayerQueueChange::CurrentChanged);
    }
    auto at = position(order_.as_slice(), *current_entry_id());
    return advance_to(order_[at == usize() ? order_.len() - usize(1) : at - usize(1)]);
}

auto PlayerQueue::set_loop_mode(LoopMode mode) -> Result<PlayerQueueChange, String> {
    switch (mode) {
    case LoopMode::None:
    case LoopMode::Track:
    case LoopMode::Playlist: break;
    default: return Err("invalid player queue loop mode"_Str);
    }
    if (mode == loop_mode_) return Ok(PlayerQueueChange::Unchanged);
    auto revision = rstd_try(next_revision());
    loop_mode_    = mode;
    revision_     = revision;
    return Ok(PlayerQueueChange::Unchanged);
}

auto PlayerQueue::set_shuffle(bool value) -> Result<PlayerQueueChange, String> {
    if (value == shuffle_) return Ok(PlayerQueueChange::Unchanged);
    auto     revision = rstd_try(next_revision());
    Vec<u64> order;
    if (value) {
        order = rstd_try(make_order(entry_ids_.as_slice(), current_entry_id(), true));
    } else {
        order = entry_ids_.clone();
    }
    order_   = rstd::move(order);
    shuffle_ = value;
    reset_history();
    revision_ = revision;
    return Ok(PlayerQueueChange::Unchanged);
}

auto PlayerQueue::set_random_mode(bool value) -> Result<PlayerQueueChange, String> {
    if (value == random_mode_) return Ok(PlayerQueueChange::Unchanged);
    auto revision = rstd_try(next_revision());
    random_mode_  = value;
    reset_history();
    revision_ = revision;
    return Ok(PlayerQueueChange::Unchanged);
}
} // namespace qcm::backend
