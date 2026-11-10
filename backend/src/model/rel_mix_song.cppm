export module qcm.backend:model.rel_mix_song;
export import rstd;
export import :db.timestamp;
import liorm;
using namespace rstd::prelude;

export namespace qcm::backend
{
struct [[pmacro::derive("liorm::table"), pmacro::helper("liorm::table", name("rel_mix_song"),
                                                        primary("rel_mix_song_pk"))]] RelMixSong {
    [[pmacro::helper("liorm::table", storage(integer), primary)]]
    i64 id;
    [[pmacro::helper("liorm::table", storage(integer))]]
    i64 song_id;
    [[pmacro::helper("liorm::table", storage(integer))]]
    i64 mix_id;
    [[pmacro::helper("liorm::table", storage(integer))]]
    i64 order_idx;
    [[pmacro::helper("liorm::table", storage(integer))]]
    Timestamp update_at;
};
} // namespace qcm::backend
