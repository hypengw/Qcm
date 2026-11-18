module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
module qcm.backend;
import :entity;
import :convert;
import :db.sync.run;
import :db.schema;
import :db.error.query;
import :model.album;
import :model.artist;
import :model.mix;
import :model.mix_model;
import :model.song;
import :db.codec;
import :model.item;
import :model.dynamic;
import :model.rel_song_artist;
import :model.rel_album_artist;
import liorm.sqlito;
using rstd::collections::BTreeMap;
using rstd::collections::BTreeSet;
using rstd::collections::HashMap;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace entity_query = liorm::query;

namespace qcm::backend
{
namespace entity_detail
{
template<class Model, auto Key = &Model::id, class Database>
auto read_models(Database& db, slice<i64> ids) -> QueryResult<Vec<Model>> {
    if (ids.is_empty()) return Ok(Vec<Model> {});
    constexpr entity_query::ProjectionLayout<entity_query::ModelGroup<Model>> projection;
    auto                                                                      query =
        rstd_try(entity_query::try_compile_expanded_planned([&](entity_query::QueryBuilder& b) {
            auto table = b.use(database_tables).template table<Model>();
            auto keys  = b.param_group<i64>("ids"_str, ids.len());
            return b.select(entity_query::project(projection, table).as_slice())
                .from(table.source())
                .where(table.template col<Key>().in(keys))
                .order_by(table.template col<&Model::id>().asc());
        }));
    auto statement = rstd_try(liorm::sqlito::prepare(db, rstd::move(query)));
    rstd_try(statement.bind_group("ids"_str, ids));
    Vec<Model> result;
    while (rstd_try(statement.step()) == sqlito::Step::Row) {
        auto reader = statement.reader();
        result.push(rstd_try(liorm::decode_model(projection.template group<0>(), reader)));
    }
    return Ok(rstd::move(result));
}

auto reference_ids(const BTreeSet<i64>& values) -> Vec<i64> {
    Vec<i64> ids;
    for (auto value : values.iter()) ids.push(i64(*value));
    return ids;
}
using ArtistRelations = HashMap<i64, Vec<i64>>;

template<auto Owner, class Relation>
auto group_artists(const Vec<Relation>& rows) -> ArtistRelations {
    ArtistRelations grouped;
    for (const auto& row : rows.as_slice()) {
        auto owner = row.*Owner;
        if (auto artists = grouped.get_mut(owner)) {
            (*artists)->push(i64(row.artist_id));
        } else {
            Vec<i64> values;
            values.push(i64(row.artist_id));
            (void)grouped.insert(owner, rstd::move(values));
        }
    }
    return grouped;
}
auto related_artists(const ArtistRelations& relations, i64 id) -> slice<i64> {
    auto artists = relations.get(id);
    return artists.is_some() ? (*artists)->as_slice() : slice<i64> {};
}
auto read_stamp(const ProjectionSnapshot& snapshot, i64 owner)
    -> Box<p_qcm::p_msg::p_entity::M_ReadStamp> {
    auto stamp     = Box<p_qcm::p_msg::p_entity::M_ReadStamp>::make();
    stamp->f_owner = owner;
    stamp->f_epoch = u64();
    for (const auto& epoch : snapshot.epochs.as_slice()) {
        if (rstd::get<0>(epoch) == owner) {
            stamp->f_epoch = rstd::get<1>(epoch);
            break;
        }
    }
    stamp->f_revision = snapshot.revision;
    return stamp;
}
auto item_state(bool value) -> Box<p_qcm::p_msg::p_entity::M_ItemState> {
    auto state        = Box<p_qcm::p_msg::p_entity::M_ItemState>::make();
    state->f_favorite = Some(value);
    return state;
}
auto removal(i64 id, p_qcm::p_msg::p_model::E_ItemType type, const ProjectionSnapshot& snapshot)
    -> Box<p_qcm::p_msg::p_entity::M_Removal> {
    auto result     = Box<p_qcm::p_msg::p_entity::M_Removal>::make();
    result->f_id    = id;
    result->f_type  = i32(static_cast<int>(type));
    result->f_stamp = Some(read_stamp(snapshot, i64()));
    return result;
}
auto song_update(Song song, const ProjectionSnapshot& snapshot, i64 owner, bool favorite,
                 slice<i64> artist_ids) -> Box<SongUpdate> {
    auto update     = Box<SongUpdate>::make();
    auto id         = Box<p_qcm::p_msg::p_id::M_SongId>::make();
    id->f_value     = song.id;
    update->f_id    = Some(rstd::move(id));
    update->f_stamp = Some(read_stamp(snapshot, owner));
    auto relations  = Box<p_qcm::p_msg::p_entity::M_SongRelations>::make();
    auto album      = Box<p_qcm::p_msg::p_entity::M_AlbumLink>::make();
    if (song.album_id.is_some()) {
        auto album_id     = Box<p_qcm::p_msg::p_id::M_AlbumId>::make();
        album_id->f_value = *song.album_id;
        album->f_id       = Some(rstd::move(album_id));
    }
    relations->f_album = Some(rstd::move(album));
    auto artists       = Box<p_qcm::p_msg::p_id::M_ArtistIds>::make();
    for (const auto& value : artist_ids) artists->f_values.push(i64(value.to_primitive()));
    relations->f_artists = Some(rstd::move(artists));
    update->f_relations  = Some(rstd::move(relations));
    update->f_state      = Some(item_state(favorite));
    update->f_fields =
        Some(rstd::into<Box<p_qcm::p_msg::p_entity::M_SongFields>>(rstd::move(song)));
    return update;
}
auto album_update(Album album, const ProjectionSnapshot& snapshot, i64 owner, bool favorite,
                  slice<i64> artist_ids) -> Box<AlbumUpdate> {
    auto update      = Box<AlbumUpdate>::make();
    auto id          = Box<p_qcm::p_msg::p_id::M_AlbumId>::make();
    id->f_value      = album.id;
    update->f_id     = Some(rstd::move(id));
    update->f_stamp  = Some(read_stamp(snapshot, owner));
    auto fields      = rstd::into<Box<p_qcm::p_msg::p_entity::M_AlbumFields>>(rstd::move(album));
    update->f_fields = Some(rstd::move(fields));
    auto relations   = Box<p_qcm::p_msg::p_entity::M_AlbumRelations>::make();
    auto artists     = Box<p_qcm::p_msg::p_id::M_ArtistIds>::make();
    for (const auto& value : artist_ids) artists->f_values.push(i64(value.to_primitive()));
    relations->f_artists = Some(rstd::move(artists));
    update->f_relations  = Some(rstd::move(relations));
    update->f_state      = Some(item_state(favorite));
    return update;
}
auto artist_update(Artist artist, const ProjectionSnapshot& snapshot, i64 owner, bool favorite)
    -> Box<ArtistUpdate> {
    auto update      = Box<ArtistUpdate>::make();
    auto id          = Box<p_qcm::p_msg::p_id::M_ArtistId>::make();
    id->f_value      = artist.id;
    update->f_id     = Some(rstd::move(id));
    update->f_stamp  = Some(read_stamp(snapshot, owner));
    auto fields      = rstd::into<Box<p_qcm::p_msg::p_entity::M_ArtistFields>>(rstd::move(artist));
    update->f_fields = Some(rstd::move(fields));
    update->f_state  = Some(item_state(favorite));
    return update;
}
auto mix_update(Mix mix, const ProjectionSnapshot& snapshot) -> Box<MixUpdate> {
    auto update      = Box<MixUpdate>::make();
    auto id          = Box<p_qcm::p_msg::p_id::M_MixId>::make();
    id->f_value      = mix.id;
    update->f_id     = Some(rstd::move(id));
    update->f_stamp  = Some(read_stamp(snapshot, i64()));
    auto fields      = rstd::into<Box<p_qcm::p_msg::p_entity::M_MixFields>>(rstd::move(mix));
    update->f_fields = Some(rstd::move(fields));
    return update;
}

struct References {
    BTreeSet<i64> songs, albums, artists, mixes;
    auto          add(BTreeSet<i64>& list, i64 id) -> void { (void)list.insert(id); }
    template<class Ids>
    auto add(BTreeSet<i64>& list, const Option<Box<Ids>>& ids) -> void {
        if (ids.is_none()) return;
        for (auto id : (*ids)->f_values.as_slice()) add(list, i64(id));
    }
};
auto collect(const p_qcm::p_msg::M_QcmMessage& response, References& refs) -> void {
    if (response.o_payload.is_none()) return;
    RSTD_MATCH(*response.o_payload) {
        RSTD_CASE_PAYLOAD(get_mixs_rsp, rsp) { refs.add(refs.mixes, rsp->f_ids); }
        RSTD_CASE_PAYLOAD(get_mix_rsp, rsp) {
            if (rsp->f_id.is_some()) refs.add(refs.mixes, (*rsp->f_id)->f_value);
        }
        RSTD_CASE_PAYLOAD(get_mix_songs_rsp, rsp) {
            if (rsp->f_mix_id.is_some()) refs.add(refs.mixes, (*rsp->f_mix_id)->f_value);
            refs.add(refs.songs, rsp->f_ids);
        }
        RSTD_CASE_PAYLOAD(get_songs_by_id_rsp, rsp) { refs.add(refs.songs, rsp->f_ids); }
        RSTD_CASE_PAYLOAD(get_radio_preview_rsp, rsp) {
            if (rsp->f_song_id.is_some()) refs.add(refs.songs, (*rsp->f_song_id)->f_value);
        }
        RSTD_CASE_PAYLOAD(get_album_rsp, rsp) {
            if (rsp->f_id.is_some()) refs.add(refs.albums, (*rsp->f_id)->f_value);
            refs.add(refs.songs, rsp->f_song_ids);
        }
        RSTD_CASE_PAYLOAD(get_artist_rsp, rsp) {
            if (rsp->f_id.is_some()) refs.add(refs.artists, (*rsp->f_id)->f_value);
        }
        RSTD_CASE_PAYLOAD(get_albums_rsp, rsp) { refs.add(refs.albums, rsp->f_ids); }
        RSTD_CASE_PAYLOAD(get_artists_rsp, rsp) { refs.add(refs.artists, rsp->f_ids); }
        RSTD_CASE_PAYLOAD(get_album_artists_rsp, rsp) { refs.add(refs.artists, rsp->f_ids); }
        RSTD_CASE_PAYLOAD(search_rsp, rsp) {
            if (rsp->f_albums.is_some()) refs.add(refs.albums, (*rsp->f_albums)->f_ids);
            if (rsp->f_artists.is_some()) refs.add(refs.artists, (*rsp->f_artists)->f_ids);
            if (rsp->f_songs.is_some()) refs.add(refs.songs, (*rsp->f_songs)->f_ids);
        }
        RSTD_CASE_PAYLOAD(create_mix_rsp, rsp) {
            if (rsp->f_id.is_some()) refs.add(refs.mixes, (*rsp->f_id)->f_value);
        }
        RSTD_CASE_PAYLOAD(mix_manipulate_rsp, rsp) {
            if (rsp->f_id.is_some()) refs.add(refs.mixes, (*rsp->f_id)->f_value);
        }
        RSTD_CASE_PAYLOAD(get_remote_mixs_rsp, rsp) { refs.add(refs.mixes, rsp->f_ids); }
        RSTD_CASE_PAYLOAD(get_song_ids_rsp, rsp) { refs.add(refs.songs, rsp->f_ids); }
        RSTD_CASE_PAYLOAD(get_artist_album_rsp, rsp) { refs.add(refs.albums, rsp->f_ids); }
        RSTD_CASE_PAYLOAD(get_home_block_items_rsp, rsp) {
            if (rsp->o_content.is_none()) return;
            RSTD_MATCH(*rsp->o_content) {
                RSTD_CASE_PAYLOAD(albums, value) { refs.add(refs.albums, value->f_ids); }
                RSTD_CASE_PAYLOAD(mixes, value) { refs.add(refs.mixes, value->f_ids); }
                RSTD_CASE_PAYLOAD(artists, value) { refs.add(refs.artists, value->f_ids); }
                RSTD_CASE_PAYLOAD(songs, value) { refs.add(refs.songs, value->f_ids); }
            }
        }
        RSTD_CASE_PAYLOAD(get_home_recommendations_rsp, rsp) {
            for (const auto& provider : rsp->f_providers.as_slice()) {
                for (const auto& card : provider->f_for_you.as_slice()) {
                    if (card->o_target.is_none()) continue;
                    RSTD_MATCH(*card->o_target) {
                        RSTD_CASE_PAYLOAD(mix, value) {
                            if (value->f_id.is_some())
                                refs.add(refs.mixes, (*value->f_id)->f_value);
                        }
                        RSTD_CASE_PAYLOAD(album, value) {
                            if (value->f_id.is_some())
                                refs.add(refs.albums, (*value->f_id)->f_value);
                        }
                        RSTD_CASE_PAYLOAD(artist, value) {
                            if (value->f_id.is_some())
                                refs.add(refs.artists, (*value->f_id)->f_value);
                        }
                        RSTD_CASE_PAYLOAD(song, value) {
                            if (value->f_id.is_some())
                                refs.add(refs.songs, (*value->f_id)->f_value);
                        }
                        RSTD_CASE(radio) {}
                    }
                }
            }
        }
        RSTD_CASE_DEFAULT { break; }
    }
}
auto collect_request(const p_qcm::p_msg::M_QcmMessage& request, References& refs) -> void {
    if (request.o_payload.is_none()) return;
    RSTD_MATCH(*request.o_payload) {
        RSTD_CASE_PAYLOAD(set_favorite_req, favorite) {
            using ItemType = p_qcm::p_msg::p_model::E_ItemType;
            auto type      = favorite->f_item_type;
            if (type == i32(static_cast<int>(ItemType::v_ITEM_TYPE_SONG)))
                refs.add(refs.songs, favorite->f_id);
            else if (type == i32(static_cast<int>(ItemType::v_ITEM_TYPE_ALBUM)))
                refs.add(refs.albums, favorite->f_id);
            else if (type == i32(static_cast<int>(ItemType::v_ITEM_TYPE_ARTIST)) ||
                     type == i32(static_cast<int>(ItemType::v_ITEM_TYPE_ALBUMARTIST)))
                refs.add(refs.artists, favorite->f_id);
        }
        RSTD_CASE_PAYLOAD(mix_manipulate_req, entry) {
            if (entry->f_id.is_some()) refs.add(refs.mixes, (*entry->f_id)->f_value);
        }
        RSTD_CASE_DEFAULT { break; }
    }
}
} // namespace entity_detail

auto qcm_entity_batch(sqlito::Transaction& db, const ProjectionSnapshot& snapshot,
                      const p_qcm::p_msg::M_QcmMessage& response,
                      const p_qcm::p_msg::M_QcmMessage* request) -> QueryResult<Box<EntityBatch>> {
    using namespace entity_detail;
    auto batch        = Box<EntityBatch>::make();
    batch->f_protocol = u32(2);
    batch->f_instance = snapshot.instance.clone();
    BTreeMap<i64, u64> epochs;
    for (const auto& epoch : snapshot.epochs.as_slice())
        (void)epochs.insert(rstd::get<0>(epoch), rstd::get<1>(epoch));
    for (auto epoch : epochs.iter()) {
        auto stamp        = Box<p_qcm::p_msg::p_entity::M_ReadStamp>::make();
        stamp->f_owner    = *rstd::get<0>(epoch);
        stamp->f_epoch    = *rstd::get<1>(epoch);
        stamp->f_revision = snapshot.revision;
        batch->f_invalidations.push(rstd::move(stamp));
    }
    References refs;
    collect(response, refs);
    if (request != nullptr) collect_request(*request, refs);
    if (refs.songs.is_empty() && refs.albums.is_empty() && refs.artists.is_empty() &&
        refs.mixes.is_empty())
        return Ok(rstd::move(batch));

    auto song_ids = reference_ids(refs.songs);
    auto songs    = rstd_try(read_models<Song>(db, song_ids.as_slice()));
    for (const auto& song : songs.as_slice())
        if (song.album_id.is_some()) refs.add(refs.albums, *song.album_id);
    auto song_artists =
        rstd_try((read_models<RelSongArtist, &RelSongArtist::song_id>(db, song_ids.as_slice())));
    auto album_ids     = reference_ids(refs.albums);
    auto album_artists = rstd_try(
        (read_models<RelAlbumArtist, &RelAlbumArtist::album_id>(db, album_ids.as_slice())));
    for (const auto& relation : song_artists.as_slice()) refs.add(refs.artists, relation.artist_id);
    for (const auto& relation : album_artists.as_slice())
        refs.add(refs.artists, relation.artist_id);
    auto artist_ids = reference_ids(refs.artists);
    auto mix_ids    = reference_ids(refs.mixes);
    auto albums     = rstd_try(read_models<Album>(db, album_ids.as_slice()));
    auto artists    = rstd_try(read_models<Artist>(db, artist_ids.as_slice()));
    auto mixes      = rstd_try(read_models<Mix>(db, mix_ids.as_slice()));

    auto item_ids = refs.songs.clone();
    for (auto id : refs.albums.iter()) (void)item_ids.insert(*id);
    for (auto id : refs.artists.iter()) (void)item_ids.insert(*id);
    auto               ids      = reference_ids(item_ids);
    auto               items    = rstd_try(read_models<Item>(db, ids.as_slice()));
    auto               dynamics = rstd_try(read_models<Dynamic>(db, ids.as_slice()));
    HashMap<i64, i64>  owners;
    HashMap<i64, bool> favorites;
    for (const auto& item : items.as_slice()) (void)owners.insert(item.id, item.provider_id);
    for (const auto& dynamic : dynamics.as_slice())
        (void)favorites.insert(dynamic.id, dynamic.favorite_at.is_some());
    auto owner = [&](i64 id) -> QueryResult<i64> {
        auto found = owners.get(id);
        if (found.is_none()) return Err(QueryError::invalid("entity owner missing"_str));
        return Ok(**found);
    };
    auto favorite = [&](i64 id) -> bool {
        auto found = favorites.get(id);
        return found.is_some() && **found;
    };
    auto song_relations  = group_artists<&RelSongArtist::song_id>(song_artists);
    auto album_relations = group_artists<&RelAlbumArtist::album_id>(album_artists);
    for (auto& song : songs) {
        auto id = song.id;
        (void)refs.songs.remove(id);
        batch->f_songs.push(song_update(rstd::move(song),
                                        snapshot,
                                        rstd_try(owner(id)),
                                        favorite(id),
                                        related_artists(song_relations, id)));
    }
    for (auto& album : albums) {
        auto id = album.id;
        (void)refs.albums.remove(id);
        batch->f_albums.push(album_update(rstd::move(album),
                                          snapshot,
                                          rstd_try(owner(id)),
                                          favorite(id),
                                          related_artists(album_relations, id)));
    }
    for (auto& artist : artists) {
        auto id = artist.id;
        (void)refs.artists.remove(id);
        batch->f_artists.push(
            artist_update(rstd::move(artist), snapshot, rstd_try(owner(id)), favorite(id)));
    }
    for (auto& mix : mixes) {
        (void)refs.mixes.remove(mix.id);
        batch->f_mixes.push(mix_update(rstd::move(mix), snapshot));
    }
    auto missing = [&](const BTreeSet<i64>& ids, p_qcm::p_msg::p_model::E_ItemType type) {
        for (auto id : ids.iter()) batch->f_removals.push(removal(*id, type, snapshot));
    };
    missing(refs.songs, p_qcm::p_msg::p_model::E_ItemType::v_ITEM_TYPE_SONG);
    missing(refs.albums, p_qcm::p_msg::p_model::E_ItemType::v_ITEM_TYPE_ALBUM);
    missing(refs.artists, p_qcm::p_msg::p_model::E_ItemType::v_ITEM_TYPE_ARTIST);
    missing(refs.mixes, p_qcm::p_msg::p_model::E_ItemType::v_ITEM_TYPE_MIX);
    return Ok(rstd::move(batch));
}
} // namespace qcm::backend
