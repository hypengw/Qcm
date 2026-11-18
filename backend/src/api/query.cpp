module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
module qcm.backend;
import :api.query;
import :identity;
import :db.query.input;
import :db.query.album;
import :db.query.artist_albums;
import :db.query.search;
import :db.query.artist;
import :db.mix.remote_query;
import :db.query.song;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend
{
namespace query_detail
{
auto type_text_filter(const p_qcm::p_msg::p_filter::M_TypeStringFilter& value) -> TypeTextFilter {
    return {
        value.f_value.clone(),
        rstd::try_into<TypeCondition>(value.f_condition).unwrap_or(TypeCondition::Unspecified)
    };
}

auto filter_logics(const Vec<Box<p_qcm::p_msg::p_filter::M_FilterLogic>>& values)
    -> Vec<FilterLogic> {
    Vec<FilterLogic> result;
    for (const auto& value : values)
        result.push({ value->f_group_a,
                      value->f_group_b,
                      rstd::try_into<LogicOp>(value->f_op).unwrap_or(LogicOp::Unspecified) });
    return result;
}

auto album_filters(const Vec<Box<p_qcm::p_msg::p_filter::M_AlbumFilter>>& values,
                   const Vec<Box<p_qcm::p_msg::p_filter::M_FilterLogic>>& logics)
    -> ProcessResult<QueryFilters<AlbumFilter>> {
    QueryFilters<AlbumFilter> result;
    result.logics = filter_logics(logics);
    for (const auto& value : values) {
        if (value->o_payload.is_none()) continue;
        auto group = value->f_group;
        RSTD_MATCH(*value->o_payload) {
            RSTD_CASE_PAYLOAD(title_filter, field) {
                result.values.push({ group, AlbumFilter::Name(text_filter(*field)) });
            }
            RSTD_CASE_PAYLOAD(track_filter, field) {
                result.values.push({ group, AlbumFilter::TrackCount(number_filter(*field)) });
            }
            RSTD_CASE_PAYLOAD(artist_name_filter, field) {
                result.values.push({ group, AlbumFilter::ArtistName(text_filter(*field)) });
            }
            RSTD_CASE_PAYLOAD(artist_id_filter, field) {
                auto id = rstd_try(required_id(field->f_value, "artist_filter"_str));
                result.values.push({ group, AlbumFilter::ArtistId(id) });
            }
            RSTD_CASE_PAYLOAD(album_artist_id_filter, field) {
                auto id = rstd_try(required_id(field->f_value, "album_artist_filter"_str));
                result.values.push({ group, AlbumFilter::AlbumArtistId(id) });
            }
            RSTD_CASE_PAYLOAD(year_filter, field) {
                result.values.push({ group, AlbumFilter::Year(number_filter(*field)) });
            }
            RSTD_CASE_PAYLOAD(added_date_filter, field) {
                result.values.push({ group, AlbumFilter::AddedAt(date_filter(*field)) });
            }
            RSTD_CASE_PAYLOAD(duration_filter, field) {
                result.values.push({ group, AlbumFilter::Duration(number_filter(*field)) });
            }
            RSTD_CASE_PAYLOAD(type_filter, field) {
                result.values.push({ group, AlbumFilter::Type(type_filter(*field)) });
            }
            RSTD_CASE_PAYLOAD(disc_count_filter, field) {
                result.values.push({ group, AlbumFilter::DiscCount(number_filter(*field)) });
            }
            RSTD_CASE_PAYLOAD(last_played_at_filter, field) {
                result.values.push({ group, AlbumFilter::LastPlayedAt(date_filter(*field)) });
            }
        }
    }
    return Ok(rstd::move(result));
}

auto album_input(const p_qcm::p_msg::M_GetAlbumsReq& entry) -> ProcessResult<AlbumQuery> {
    auto filters   = rstd_try(album_filters(entry.f_filters, entry.f_filter_logics));
    auto libraries = rstd_try(checked_ids(entry.f_library_id, "library_ids"_str));
    return Ok(AlbumQuery { rstd::move(libraries),
                           rstd::move(filters),
                           rstd::try_into<AlbumSort>(entry.f_sort).unwrap_or(AlbumSort::Title),
                           entry.f_sort_asc,
                           page_of(entry.f_page, entry.f_page_size) });
}

auto artist_album_input(const p_qcm::p_msg::M_GetArtistAlbumReq& entry)
    -> ProcessResult<ArtistAlbumQuery> {
    auto id = rstd_try(required_id(entry.f_id, "artist_id"_str));
    return Ok(ArtistAlbumQuery {
        id,
        rstd::try_into<AlbumSort>(entry.f_sort).unwrap_or(AlbumSort::PublishTime),
        entry.f_sort_asc,
        page_of(entry.f_page, entry.f_page_size) });
}

auto song_filters(const Vec<Box<p_qcm::p_msg::p_filter::M_SongFilter>>&  values,
                  const Vec<Box<p_qcm::p_msg::p_filter::M_FilterLogic>>& logics)
    -> ProcessResult<QueryFilters<SongFilter>> {
    QueryFilters<SongFilter> filters;
    filters.logics = filter_logics(logics);
    for (const auto& value : values) {
        if (value->o_payload.is_none()) continue;
        auto group = value->f_group;
        RSTD_MATCH(*value->o_payload) {
            RSTD_CASE_PAYLOAD(album_id_filter, field) {
                auto id = rstd_try(required_id(field->f_value, "album_filter"_str));
                filters.values.push({ group, SongFilter::AlbumId(id) });
            }
            RSTD_CASE_PAYLOAD(mix_id_filter, field) {
                auto id = rstd_try(required_id(field->f_value, "mix_filter"_str));
                filters.values.push({ group, SongFilter::MixId(id) });
            }
        }
    }
    return Ok(rstd::move(filters));
}

auto prepare_song_query(const p_qcm::p_msg::M_GetSongIdsReq& entry) -> ProcessResult<SongIdsQuery> {
    auto songs     = rstd_try(song_filters(entry.f_filters, entry.f_filter_logics));
    auto albums    = rstd_try(album_filters(entry.f_album_filters, entry.f_album_filter_logics));
    auto libraries = rstd_try(checked_ids(entry.f_library_ids, "library_ids"_str));
    auto sort = rstd_try(rstd::try_into<SongSort>(entry.f_sort), ProcessError::NotImplemented());
    if (! entry.f_filters.is_empty())
        return Ok(SongIdsQuery::Songs(FilteredSongQuery { rstd::move(songs), sort, entry.f_asc }));
    auto album_sort =
        rstd_try(rstd::try_into<AlbumSort>(entry.f_album_sort), ProcessError::NotImplemented());
    return Ok(SongIdsQuery::Albums(AlbumSongQuery {
        AlbumQuery { rstd::move(libraries), rstd::move(albums), album_sort, entry.f_album_asc, {} },
        sort,
        entry.f_asc }));
}

auto mix_input(const p_qcm::p_msg::M_GetMixsReq& entry) -> MixQuery {
    QueryFilters<MixFilter> filters;
    filters.logics = filter_logics(entry.f_filter_logics);
    for (const auto& value : entry.f_filters) {
        if (value->o_payload.is_none()) continue;
        auto group = value->f_group;
        RSTD_MATCH(*value->o_payload) {
            RSTD_CASE_PAYLOAD(name_filter, field) {
                filters.values.push({ group, MixFilter::Name(text_filter(*field)) });
            }
            RSTD_CASE_PAYLOAD(track_filter, field) {
                filters.values.push({ group, MixFilter::TrackCount(number_filter(*field)) });
            }
            RSTD_CASE_PAYLOAD(type_filter, field) {
                filters.values.push({ group, MixFilter::Type(type_filter(*field)) });
            }
            RSTD_CASE_DEFAULT { break; }
        }
    }
    return { rstd::move(filters), page_of(entry.f_page, entry.f_page_size) };
}

auto remote_mix_input(const p_qcm::p_msg::M_GetRemoteMixsReq& entry) -> RemoteMixQuery {
    QueryFilters<RemoteMixFilter> filters;
    filters.logics = filter_logics(entry.f_filter_logics);
    for (const auto& value : entry.f_filters) {
        if (value->o_payload.is_none()) continue;
        auto group = value->f_group;
        RSTD_MATCH(*value->o_payload) {
            RSTD_CASE_PAYLOAD(name_filter, field) {
                filters.values.push({ group, RemoteMixFilter::Name(text_filter(*field)) });
            }
            RSTD_CASE_PAYLOAD(track_filter, field) {
                filters.values.push({ group, RemoteMixFilter::TrackCount(number_filter(*field)) });
            }
            RSTD_CASE_PAYLOAD(type_filter, field) {
                filters.values.push({ group, RemoteMixFilter::Type(type_text_filter(*field)) });
            }
            RSTD_CASE_PAYLOAD(local_type_filter, field) {
                filters.values.push({ group, RemoteMixFilter::LocalType(type_filter(*field)) });
            }
        }
    }
    return { rstd::move(filters), page_of(entry.f_page, entry.f_page_size) };
}

auto search_input(const p_qcm::p_msg::M_SearchReq& entry) -> ProcessResult<Vec<SearchQuery>> {
    Vec<i64> libraries;
    if (! entry.f_query.is_empty())
        libraries = rstd_try(checked_ids(entry.f_library_id, "library_ids"_str));
    Vec<SearchQuery> queries;
    for (auto type : entry.f_types) {
        auto kind = rstd_try(rstd::try_into<SearchKind>(type),
                             ProcessError::NoSuchSearchType(rstd::format("{}", type)));
        if (! entry.f_query.is_empty())
            queries.push({ kind,
                           entry.f_query.clone(),
                           libraries.clone(),
                           page_of(entry.f_page, entry.f_page_size) });
    }
    return Ok(rstd::move(queries));
}
} // namespace query_detail

template<class Database>
auto query_get_albums(Database& db, i32 request_id, const p_qcm::p_msg::M_GetAlbumsReq& entry)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage> {
    using namespace query_detail;
    auto input  = rstd_try(album_input(entry));
    auto page   = input.page;
    auto result = rstd_try(album_query_page(db, input), ProcessError::query(rstd::move(_e)));
    auto rsp    = Box<p_qcm::p_msg::M_GetAlbumsRsp>::make();
    auto list   = Box<p_qcm::p_msg::p_id::M_AlbumIds>::make();
    for (const auto& id : result.ids.as_slice()) list->f_values.push(i64(id.to_primitive()));
    rsp->f_ids      = Some(rstd::move(list));
    rsp->f_total    = i32(static_cast<int>(result.total.to_primitive()));
    rsp->f_has_more = page.more(result.total);
    return Ok(qcm_message(request_id, rstd::move(rsp)));
}

template<class Database>
auto query_get_artists(Database& db, i32 request_id, const p_qcm::p_msg::M_GetArtistsReq& entry)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage> {
    using namespace query_detail;
    auto input = rstd_try(artist_input(entry));
    auto page  = input.page;
    auto result =
        rstd_try(artist_query_page(db, input, false), ProcessError::query(rstd::move(_e)));
    auto rsp  = Box<p_qcm::p_msg::M_GetArtistsRsp>::make();
    auto list = Box<p_qcm::p_msg::p_id::M_ArtistIds>::make();
    for (const auto& id : result.ids.as_slice()) list->f_values.push(i64(id.to_primitive()));
    rsp->f_ids      = Some(rstd::move(list));
    rsp->f_total    = i32(static_cast<int>(result.total.to_primitive()));
    rsp->f_has_more = page.more(result.total);
    return Ok(qcm_message(request_id, rstd::move(rsp)));
}

template<class Database>
auto query_get_album_artists(Database& db, i32 request_id,
                             const p_qcm::p_msg::M_GetAlbumArtistsReq& entry)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage> {
    using namespace query_detail;
    auto input  = rstd_try(artist_input(entry));
    auto page   = input.page;
    auto result = rstd_try(artist_query_page(db, input, true), ProcessError::query(rstd::move(_e)));
    auto rsp    = Box<p_qcm::p_msg::M_GetAlbumArtistsRsp>::make();
    auto list   = Box<p_qcm::p_msg::p_id::M_ArtistIds>::make();
    for (const auto& id : result.ids.as_slice()) list->f_values.push(i64(id.to_primitive()));
    rsp->f_ids      = Some(rstd::move(list));
    rsp->f_total    = i32(static_cast<int>(result.total.to_primitive()));
    rsp->f_has_more = page.more(result.total);
    return Ok(qcm_message(request_id, rstd::move(rsp)));
}

template<class Database>
auto query_get_artist_album(Database& db, i32 request_id,
                            const p_qcm::p_msg::M_GetArtistAlbumReq& entry)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage> {
    using namespace query_detail;
    auto input     = rstd_try(artist_album_input(entry));
    auto artist_id = input.artist_id;
    auto page      = input.page;
    auto result =
        rstd_try(artist_album_page(db, rstd::move(input)), ProcessError::query(rstd::move(_e)));
    if (result.is_none()) return Err(ProcessError::NoSuchArtist(rstd::format("{}", artist_id)));
    auto total = result->total;
    auto rsp   = Box<p_qcm::p_msg::M_GetArtistAlbumRsp>::make();
    auto ids   = Box<p_qcm::p_msg::p_id::M_AlbumIds>::make();
    for (auto id : result->ids) ids->f_values.push(i64(id));
    rsp->f_ids      = Some(rstd::move(ids));
    rsp->f_total    = i32(static_cast<int>(total.to_primitive()));
    rsp->f_has_more = page.more(total);
    return Ok(qcm_message(request_id, rstd::move(rsp)));
}

template<class Database>
auto query_get_song_ids_prepared(Database& db, i32 request_id, const SongIdsQuery& input)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage> {
    auto result = rstd_try(input.is_Albums() ? album_song_ids(db, input.as_Albums().query)
                                             : filtered_song_ids(db, input.as_Songs().query),
                           ProcessError::query(rstd::move(_e)));
    auto rsp    = Box<p_qcm::p_msg::M_GetSongIdsRsp>::make();
    auto ids    = Box<p_qcm::p_msg::p_id::M_SongIds>::make();
    for (const auto& id : result.as_slice()) ids->f_values.push(i64(id));
    rsp->f_ids = Some(rstd::move(ids));
    return Ok(qcm_message(request_id, rstd::move(rsp)));
}

template<class Database>
auto query_get_song_ids(Database& db, i32 request_id, const p_qcm::p_msg::M_GetSongIdsReq& entry)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage> {
    auto input = rstd_try(query_detail::prepare_song_query(entry));
    return query_get_song_ids_prepared(db, request_id, input);
}

template<class Database>
auto query_get_remote_mixs(Database& db, i32 request_id,
                           const p_qcm::p_msg::M_GetRemoteMixsReq& entry)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage> {
    using namespace query_detail;
    auto page   = page_of(entry.f_page, entry.f_page_size);
    auto result = rstd_try(remote_mix_query_page(db, remote_mix_input(entry)),
                           ProcessError::query(rstd::move(_e)));
    auto rsp    = Box<p_qcm::p_msg::M_GetRemoteMixsRsp>::make();
    auto ids    = Box<p_qcm::p_msg::p_id::M_MixIds>::make();
    for (const auto& id : result.ids.as_slice()) ids->f_values.push(i64(id));
    rsp->f_ids      = Some(rstd::move(ids));
    rsp->f_total    = i32(static_cast<int>(result.total.to_primitive()));
    rsp->f_has_more = page.more(result.total);
    return Ok(qcm_message(request_id, rstd::move(rsp)));
}

template<class Database>
auto query_search(Database& db, i32 request_id, const p_qcm::p_msg::M_SearchReq& entry)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage> {
    using namespace query_detail;
    auto queries      = rstd_try(search_input(entry));
    auto rsp          = Box<p_qcm::p_msg::M_SearchRsp>::make();
    auto search_table = [&](SearchQuery input) -> ProcessResult<empty> {
        auto kind = input.kind;
        auto page = input.page;
        auto result =
            rstd_try(search_page(db, rstd::move(input)), ProcessError::query(rstd::move(_e)));
        auto  total    = result.total;
        auto& ids      = result.ids;
        auto  has_more = page.more(total);
        if (kind == SearchKind::Album) {
            auto sub  = Box<p_qcm::p_msg::M_GetAlbumsRsp>::make();
            auto list = Box<p_qcm::p_msg::p_id::M_AlbumIds>::make();
            for (const auto& id : ids.as_slice()) list->f_values.push(i64(id.to_primitive()));
            sub->f_ids      = Some(rstd::move(list));
            sub->f_total    = i32(static_cast<int>(total.to_primitive()));
            sub->f_has_more = has_more;
            rsp->f_albums   = Some(rstd::move(sub));
            return Ok(empty {});
        }
        if (kind == SearchKind::Artist) {
            auto sub  = Box<p_qcm::p_msg::M_GetArtistsRsp>::make();
            auto list = Box<p_qcm::p_msg::p_id::M_ArtistIds>::make();
            for (const auto& id : ids.as_slice()) list->f_values.push(i64(id.to_primitive()));
            sub->f_ids      = Some(rstd::move(list));
            sub->f_total    = i32(static_cast<int>(total.to_primitive()));
            sub->f_has_more = has_more;
            rsp->f_artists  = Some(rstd::move(sub));
            return Ok(empty {});
        }
        auto sub       = Box<p_qcm::p_msg::M_GetSongsRsp>::make();
        auto song_list = Box<p_qcm::p_msg::p_id::M_SongIds>::make();
        for (const auto& id : ids.as_slice()) song_list->f_values.push(i64(id.to_primitive()));
        sub->f_ids      = Some(rstd::move(song_list));
        sub->f_total    = i32(static_cast<int>(total.to_primitive()));
        sub->f_has_more = has_more;
        rsp->f_songs    = Some(rstd::move(sub));
        return Ok(empty {});
    };
    for (auto& query : queries) rstd_try(search_table(rstd::move(query)));
    return Ok(qcm_message(request_id, rstd::move(rsp)));
}

template auto query_get_albums(
    sqlito::Connection&, i32, const p_qcm::p_msg::M_GetAlbumsReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
template auto query_get_albums(
    sqlito::Transaction&, i32, const p_qcm::p_msg::M_GetAlbumsReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
template auto query_get_artists(
    sqlito::Connection&, i32, const p_qcm::p_msg::M_GetArtistsReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
template auto query_get_artists(
    sqlito::Transaction&, i32, const p_qcm::p_msg::M_GetArtistsReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
template auto query_get_album_artists(
    sqlito::Connection&, i32, const p_qcm::p_msg::M_GetAlbumArtistsReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
template auto query_get_album_artists(
    sqlito::Transaction&, i32, const p_qcm::p_msg::M_GetAlbumArtistsReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
template auto query_get_artist_album(
    sqlito::Connection&, i32, const p_qcm::p_msg::M_GetArtistAlbumReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
template auto query_get_artist_album(
    sqlito::Transaction&, i32, const p_qcm::p_msg::M_GetArtistAlbumReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
template auto query_get_song_ids_prepared(
    sqlito::Connection&, i32, const SongIdsQuery&) -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
template auto query_get_song_ids_prepared(
    sqlito::Transaction&, i32, const SongIdsQuery&) -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
template auto query_get_song_ids(
    sqlito::Connection&, i32, const p_qcm::p_msg::M_GetSongIdsReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
template auto query_get_song_ids(
    sqlito::Transaction&, i32, const p_qcm::p_msg::M_GetSongIdsReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
template auto query_get_remote_mixs(
    sqlito::Connection&, i32, const p_qcm::p_msg::M_GetRemoteMixsReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
template auto query_get_remote_mixs(
    sqlito::Transaction&, i32, const p_qcm::p_msg::M_GetRemoteMixsReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
template auto query_search(
    sqlito::Connection&, i32, const p_qcm::p_msg::M_SearchReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
template auto query_search(
    sqlito::Transaction&, i32, const p_qcm::p_msg::M_SearchReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
} // namespace qcm::backend
