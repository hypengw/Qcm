export module qcm.backend:playing;
export import rstd;
using rstd::collections::HashMap;
using rstd::sync::Mutex;
using namespace rstd::prelude;

export namespace qcm::backend
{
struct PlayingInfo {
    i64    song_id;
    String native_id;
    auto   clone() const -> PlayingInfo { return { song_id, native_id.clone() }; }
};
enum class PlayingAction
{
    Unchanged,
    Play,
    Stop
};
class PlayingState {
    Mutex<HashMap<i64, PlayingInfo>> entries_;

public:
    auto set(i64 provider, PlayingInfo info) const -> void {
        auto entries = entries_.lock().unwrap();
        entries->insert(provider, rstd::move(info));
    }
    auto get(i64 provider) const -> Option<PlayingInfo> {
        auto entries = entries_.lock().unwrap();
        auto info    = entries->get(provider);
        if (info.is_none()) return None();
        return Some((**info).clone());
    }
    auto clear(i64 provider) const -> void {
        auto entries = entries_.lock().unwrap();
        (void)entries->remove(provider);
    }
};
} // namespace qcm::backend
