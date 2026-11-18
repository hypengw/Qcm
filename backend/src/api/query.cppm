module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:api.query;
export import :protocol.message;
export import :error;
export import rstd;
export import sqlito;
import :identity;
import :db.query.input;
import :db.query.album;
import :db.query.artist;
import :db.timestamp;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
namespace query_detail
{
template<class T>
auto text_filter(const T& value) -> TextFilter {
    return {
        value.f_value.clone(),
        rstd::try_into<TextCondition>(value.f_condition).unwrap_or(TextCondition::Unspecified)
    };
}
template<class T>
auto number_filter(const T& value) -> NumberFilter {
    return {
        i64(value.f_value.to_primitive()),
        rstd::try_into<NumberCondition>(value.f_condition).unwrap_or(NumberCondition::Unspecified)
    };
}
template<class T>
auto date_filter(const T& value) -> DateFilter {
    return {
        Timestamp::from_millis(value.f_value),
        rstd::try_into<DateCondition>(value.f_condition).unwrap_or(DateCondition::Unspecified)
    };
}
template<class T>
auto type_filter(const T& value) -> TypeFilter {
    return {
        i64(value.f_value.to_primitive()),
        rstd::try_into<TypeCondition>(value.f_condition).unwrap_or(TypeCondition::Unspecified)
    };
}
auto type_text_filter(const p_qcm::p_msg::p_filter::M_TypeStringFilter& value) -> TypeTextFilter;
auto filter_logics(const Vec<Box<p_qcm::p_msg::p_filter::M_FilterLogic>>& values)
    -> Vec<FilterLogic>;
auto album_filters(const Vec<Box<p_qcm::p_msg::p_filter::M_AlbumFilter>>& values,
                   const Vec<Box<p_qcm::p_msg::p_filter::M_FilterLogic>>& logics)
    -> ProcessResult<QueryFilters<AlbumFilter>>;
auto album_input(const p_qcm::p_msg::M_GetAlbumsReq& entry) -> ProcessResult<AlbumQuery>;
template<class T>
auto artist_input(const T& entry) -> ProcessResult<ArtistQuery> {
    auto libraries = rstd_try(checked_ids(entry.f_library_id, "library_ids"_str));
    QueryFilters<ArtistFilter> filters;
    filters.logics = filter_logics(entry.f_filter_logics);
    for (const auto& value : entry.f_filters) {
        if (value->o_payload.is_none()) continue;
        auto group = value->f_group;
        RSTD_MATCH(*value->o_payload) {
            RSTD_CASE_PAYLOAD(name_filter, field) {
                filters.values.push({ group, ArtistFilter::Name(text_filter(*field)) });
            }
            RSTD_CASE_PAYLOAD(album_title_filter, field) {
                filters.values.push({ group, ArtistFilter::AlbumName(text_filter(*field)) });
            }
            RSTD_CASE_PAYLOAD(added_date_filter, field) {
                filters.values.push({ group, ArtistFilter::AddedAt(date_filter(*field)) });
            }
            RSTD_CASE_DEFAULT { break; }
        }
    }
    return Ok(ArtistQuery { rstd::move(libraries),
                            rstd::move(filters),
                            rstd::try_into<ArtistSort>(entry.f_sort).unwrap_or(ArtistSort::Name),
                            entry.f_sort_asc,
                            page_of(entry.f_page, entry.f_page_size) });
}
auto artist_album_input(const p_qcm::p_msg::M_GetArtistAlbumReq& entry)
    -> ProcessResult<ArtistAlbumQuery>;
auto song_filters(const Vec<Box<p_qcm::p_msg::p_filter::M_SongFilter>>&  values,
                  const Vec<Box<p_qcm::p_msg::p_filter::M_FilterLogic>>& logics)
    -> ProcessResult<QueryFilters<SongFilter>>;
auto prepare_song_query(const p_qcm::p_msg::M_GetSongIdsReq& entry) -> ProcessResult<SongIdsQuery>;
auto mix_input(const p_qcm::p_msg::M_GetMixsReq& entry) -> MixQuery;
auto remote_mix_input(const p_qcm::p_msg::M_GetRemoteMixsReq& entry) -> RemoteMixQuery;
auto search_input(const p_qcm::p_msg::M_SearchReq& entry) -> ProcessResult<Vec<SearchQuery>>;
} // namespace query_detail

template<class Database>
auto query_get_albums(Database& db, i32 request_id, const p_qcm::p_msg::M_GetAlbumsReq& entry)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;

template<class Database>
auto query_get_artists(Database& db, i32 request_id, const p_qcm::p_msg::M_GetArtistsReq& entry)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;

template<class Database>
auto query_get_album_artists(Database& db, i32 request_id,
                             const p_qcm::p_msg::M_GetAlbumArtistsReq& entry)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;

template<class Database>
auto query_get_artist_album(Database& db, i32 request_id,
                            const p_qcm::p_msg::M_GetArtistAlbumReq& entry)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
template<class Database>
auto query_get_song_ids_prepared(Database& db, i32 request_id, const SongIdsQuery& input)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
template<class Database>
auto query_get_song_ids(Database& db, i32 request_id, const p_qcm::p_msg::M_GetSongIdsReq& entry)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;

template<class Database>
auto query_get_remote_mixs(Database& db, i32 request_id,
                           const p_qcm::p_msg::M_GetRemoteMixsReq& entry)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
template<class Database>
auto query_search(Database& db, i32 request_id, const p_qcm::p_msg::M_SearchReq& entry)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;

// The SQL queries support only SQLito connections and transactions.
extern template auto query_get_albums(
    sqlito::Connection&, i32, const p_qcm::p_msg::M_GetAlbumsReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
extern template auto query_get_albums(
    sqlito::Transaction&, i32, const p_qcm::p_msg::M_GetAlbumsReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
extern template auto query_get_artists(
    sqlito::Connection&, i32, const p_qcm::p_msg::M_GetArtistsReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
extern template auto query_get_artists(
    sqlito::Transaction&, i32, const p_qcm::p_msg::M_GetArtistsReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
extern template auto query_get_album_artists(
    sqlito::Connection&, i32, const p_qcm::p_msg::M_GetAlbumArtistsReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
extern template auto query_get_album_artists(
    sqlito::Transaction&, i32, const p_qcm::p_msg::M_GetAlbumArtistsReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
extern template auto query_get_artist_album(
    sqlito::Connection&, i32, const p_qcm::p_msg::M_GetArtistAlbumReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
extern template auto query_get_artist_album(
    sqlito::Transaction&, i32, const p_qcm::p_msg::M_GetArtistAlbumReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
extern template auto query_get_song_ids_prepared(
    sqlito::Connection&, i32, const SongIdsQuery&) -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
extern template auto query_get_song_ids_prepared(
    sqlito::Transaction&, i32, const SongIdsQuery&) -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
extern template auto query_get_song_ids(
    sqlito::Connection&, i32, const p_qcm::p_msg::M_GetSongIdsReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
extern template auto query_get_song_ids(
    sqlito::Transaction&, i32, const p_qcm::p_msg::M_GetSongIdsReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
extern template auto query_get_remote_mixs(
    sqlito::Connection&, i32, const p_qcm::p_msg::M_GetRemoteMixsReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
extern template auto query_get_remote_mixs(
    sqlito::Transaction&, i32, const p_qcm::p_msg::M_GetRemoteMixsReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
extern template auto query_search(
    sqlito::Connection&, i32, const p_qcm::p_msg::M_SearchReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
extern template auto query_search(
    sqlito::Transaction&, i32, const p_qcm::p_msg::M_SearchReq&)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
} // namespace qcm::backend
