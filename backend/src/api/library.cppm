module;
#include <rstd/macro.hpp>
export module qcm.backend:api.library;
export import :event;
import :protocol.message;
import :api.query;
import :identity;
import :db.mix.query;
import :db.state.playlog;
import :db.sync.run;
import :model.mix;
import :pagination;
import :error_response;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
template<class Database>
auto album(Database& db, const QcmMessage& request, const p_qcm::p_msg::M_GetAlbumReq& entry)
    -> Result<QcmMessage, ProcessError> {
    auto id     = rstd_try(required_id(entry.f_id, "album_id"_str));
    auto exists = rstd_try(album_exists(db, id));
    if (! exists) return Err(ProcessError::NoSuchAlbum(rstd::format("{}", id)));
    auto songs        = rstd_try(album_song_ids(db, id));
    auto rsp          = Box<p_qcm::p_msg::M_GetAlbumRsp>::make();
    auto album_id     = Box<p_qcm::p_msg::p_id::M_AlbumId>::make();
    album_id->f_value = id;
    rsp->f_id         = Some(rstd::move(album_id));
    auto ids          = Box<p_qcm::p_msg::p_id::M_SongIds>::make();
    for (const auto& value : songs.as_slice()) ids->f_values.push(i64(value.to_primitive()));
    rsp->f_song_ids = Some(rstd::move(ids));
    return Ok(qcm_message(request.f_id, rstd::move(rsp)));
}
template<class Database>
auto artist(Database& db, const QcmMessage& request, const p_qcm::p_msg::M_GetArtistReq& entry)
    -> Result<QcmMessage, ProcessError> {
    auto id     = rstd_try(required_id(entry.f_id, "artist_id"_str));
    auto exists = rstd_try(artist_exists(db, id));
    if (! exists) return Err(ProcessError::NoSuchArtist(rstd::format("{}", id)));
    auto rsp           = Box<p_qcm::p_msg::M_GetArtistRsp>::make();
    auto artist_id     = Box<p_qcm::p_msg::p_id::M_ArtistId>::make();
    artist_id->f_value = id;
    rsp->f_id          = Some(rstd::move(artist_id));
    return Ok(qcm_message(request.f_id, rstd::move(rsp)));
}

auto songs_by_id_rsp(i32 id, Vec<i64> ids) -> QcmMessage {
    auto rsp   = Box<p_qcm::p_msg::M_GetSongsByIdRsp>::make();
    auto list  = Box<p_qcm::p_msg::p_id::M_SongIds>::make();
    auto count = usize();
    for (auto& value : ids.as_slice()) {
        list->f_values.push(i64(value.to_primitive()));
        count = count + usize(1);
    }
    rsp->f_ids   = Some(rstd::move(list));
    rsp->f_total = i32(static_cast<int>(count.to_primitive()));
    return qcm_message(id, rstd::move(rsp));
}
auto mix_song_order(SongSort sort) -> MixSongOrder {
    switch (sort) {
    case SongSort::TrackNumber: return MixSongOrder::TrackNumber;
    case SongSort::Title: return MixSongOrder::Title;
    case SongSort::SortTitle: return MixSongOrder::SortTitle;
    case SongSort::PublishTime: return MixSongOrder::PublishTime;
    case SongSort::Duration: return MixSongOrder::Duration;
    case SongSort::Popularity: return MixSongOrder::Popularity;
    case SongSort::Random: return MixSongOrder::Random;
    }
    return MixSongOrder::Title;
}
auto mix_songs_rsp(i32 id, i64 mix_id, const MixSongPage& page, i32 page_number, i32 page_size)
    -> QcmMessage {
    auto rsp      = Box<p_qcm::p_msg::M_GetMixSongsRsp>::make();
    auto mix      = Box<p_qcm::p_msg::p_id::M_MixId>::make();
    mix->f_value  = mix_id;
    rsp->f_mix_id = Some(rstd::move(mix));
    auto ids      = Box<p_qcm::p_msg::p_id::M_SongIds>::make();
    for (const auto& value : page.ids.as_slice()) ids->f_values.push(i64(value.to_primitive()));
    rsp->f_ids      = Some(rstd::move(ids));
    rsp->f_total    = i32(static_cast<int>(page.total.to_primitive()));
    rsp->f_has_more = page_of(page_number, page_size).more(page.total);
    return qcm_message(id, rstd::move(rsp));
}
struct MixSongsQuery {
    i64          mix_id;
    i32          page;
    i32          page_size;
    MixSongOrder order;
    bool         ascending;
};
auto prepare_mix_songs(const p_qcm::p_msg::M_GetMixSongsReq& entry)
    -> ProcessResult<MixSongsQuery> {
    auto id    = rstd_try(required_id(entry.f_id, "mix_id"_str));
    auto order = mix_song_order(rstd::try_into<SongSort>(entry.f_sort).unwrap_or(SongSort::Title));
    return Ok(MixSongsQuery { id, entry.f_page, entry.f_page_size, order, entry.f_sort_asc });
}
template<class Database>
auto mix_songs(Database& db, i32 request_id, const MixSongsQuery& input)
    -> Result<QcmMessage, ProcessError> {
    auto found = rstd_try(mix_find(db, input.mix_id));
    if (found.is_none()) return Err(ProcessError::NoSuchMix(rstd::format("{}", input.mix_id)));
    auto page = rstd_try(
        mix_song_ids(db, input.mix_id, input.page, input.page_size, input.order, input.ascending));
    return Ok(mix_songs_rsp(request_id, input.mix_id, page, input.page, input.page_size));
}
template<class Database>
auto songs_by_id(Database&, const QcmMessage& request, const p_qcm::p_msg::M_GetSongsByIdReq& entry)
    -> Result<QcmMessage, ProcessError> {
    auto values = rstd_try(checked_ids(entry.f_ids, "song_ids"_str));
    if (values.len() > usize(20000))
        return Err(ProcessError::WrongId(String::make("song IDs"_str)));
    return Ok(songs_by_id_rsp(request.f_id, rstd::move(values)));
}
auto mixs_rsp(i32 id, const MixIdPage& page, i32 page_number, i32 page_size) -> QcmMessage {
    auto rsp = Box<p_qcm::p_msg::M_GetMixsRsp>::make();
    auto ids = Box<p_qcm::p_msg::p_id::M_MixIds>::make();
    for (const auto& value : page.ids.as_slice()) ids->f_values.push(i64(value.to_primitive()));
    rsp->f_ids      = Some(rstd::move(ids));
    rsp->f_total    = i32(static_cast<int>(page.total.to_primitive()));
    rsp->f_has_more = page_of(page_number, page_size).more(page.total);
    return qcm_message(id, rstd::move(rsp));
}
template<class Database>
auto mixs(Database& db, const QcmMessage& request, const p_qcm::p_msg::M_GetMixsReq& entry)
    -> Result<QcmMessage, ProcessError> {
    auto page = rstd_try(mix_query_page(db, query_detail::mix_input(entry)));
    return Ok(mixs_rsp(request.f_id, page, entry.f_page, entry.f_page_size));
}
template<class Database>
auto mix(Database& db, const QcmMessage& request, const p_qcm::p_msg::M_GetMixReq& entry)
    -> Result<QcmMessage, ProcessError> {
    auto mix_id = rstd_try(required_id(entry.f_id, "mix_id"_str));
    auto found  = rstd_try(mix_find(db, mix_id));
    if (found.is_none()) return Err(ProcessError::NoSuchMix(rstd::format("{}", mix_id)));
    auto rsp    = Box<p_qcm::p_msg::M_GetMixRsp>::make();
    auto id     = Box<p_qcm::p_msg::p_id::M_MixId>::make();
    id->f_value = *found;
    rsp->f_id   = Some(rstd::move(id));
    return Ok(qcm_message(request.f_id, rstd::move(rsp)));
}
auto playlog(sqlito::Connection& db, SyncCoordinator& coordinator, const PlayingState& playing,
             const QcmMessage& request, const p_qcm::p_msg::M_PlaylogReq& entry)
    -> ProcessResult<QcmMessage> {
    auto song_id     = rstd_try(required_id(entry.f_song_id, "song_id"_str));
    using WireAction = p_qcm::p_msg::p_model::E_PlaylogAction;
    auto action      = entry.f_action == i32(static_cast<int>(WireAction::v_PLAYLOG_ACTION_PLAY))
                           ? PlayingAction::Play
                       : entry.f_action == i32(static_cast<int>(WireAction::v_PLAYLOG_ACTION_STOP))
                           ? PlayingAction::Stop
                           : PlayingAction::Unchanged;
    rstd_try(record_playlog(
        db, coordinator, playing, song_id, Timestamp::from_millis(entry.f_timestamp), action));
    return Ok(ok_response(request.f_id));
}
} // namespace qcm::backend
