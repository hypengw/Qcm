module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:identity;
export import :error;
import :protocol.types;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
template<class Id>
auto required_id(const Option<Box<Id>>& field, ref<str> name) -> ProcessResult<i64> {
    if (field.is_none()) return Err(ProcessError::MissingFields(String::make(name)));
    auto id = (*field)->f_value;
    if (id <= i64()) return Err(ProcessError::WrongId(String::make(name)));
    return Ok(id);
}
template<class Ids>
auto checked_ids(const Option<Box<Ids>>& field, ref<str> name) -> ProcessResult<Vec<i64>> {
    Vec<i64> ids;
    if (field.is_some()) {
        for (auto id : (*field)->f_values) {
            if (id <= i64()) return Err(ProcessError::WrongId(String::make(name)));
            ids.push(i64(id));
        }
    }
    return Ok(rstd::move(ids));
}
auto validate_album_filter_ids(const Vec<Box<p_qcm::p_msg::p_filter::M_AlbumFilter>>& filters)
    -> ProcessResult<empty> {
    for (const auto& filter : filters) {
        if (filter->o_payload.is_none()) continue;
        RSTD_MATCH(*filter->o_payload) {
            RSTD_CASE_PAYLOAD(artist_id_filter, value) {
                (void)rstd_try(required_id(value->f_value, "artist_filter"_str));
            }
            RSTD_CASE_PAYLOAD(album_artist_id_filter, value) {
                (void)rstd_try(required_id(value->f_value, "album_artist_filter"_str));
            }
            RSTD_CASE_DEFAULT { break; }
        }
    }
    return Ok(empty {});
}
auto validate_request_identity(const p_qcm::p_msg::M_QcmMessage& message) -> ProcessResult<empty> {
    if (message.o_payload.is_none()) return Ok(empty {});
    RSTD_MATCH(*message.o_payload) {
        RSTD_CASE_PAYLOAD(get_artist_req, entry) {
            (void)rstd_try(required_id(entry->f_id, "artist_id"_str));
        }
        RSTD_CASE_PAYLOAD(get_artist_album_req, entry) {
            (void)rstd_try(required_id(entry->f_id, "artist_id"_str));
        }
        RSTD_CASE_PAYLOAD(get_albums_req, entry) {
            rstd_try(validate_album_filter_ids(entry->f_filters));
        }
        RSTD_CASE_PAYLOAD(get_song_ids_req, entry) {
            for (const auto& filter : entry->f_filters) {
                if (filter->o_payload.is_none()) continue;
                RSTD_MATCH(*filter->o_payload) {
                    RSTD_CASE_PAYLOAD(album_id_filter, value) {
                        (void)rstd_try(required_id(value->f_value, "album_filter"_str));
                    }
                    RSTD_CASE_PAYLOAD(mix_id_filter, value) {
                        (void)rstd_try(required_id(value->f_value, "mix_filter"_str));
                    }
                }
            }
            rstd_try(validate_album_filter_ids(entry->f_album_filters));
        }
        RSTD_CASE_DEFAULT { break; }
    }
    return Ok(empty {});
}
} // namespace qcm::backend
