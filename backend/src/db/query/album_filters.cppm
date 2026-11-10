module;
#include <rstd/enum.hpp>
export module qcm.backend:db.query.album_filters;
export import :db.query.filter_expressions;
import :db.schema;
import :model.album;
import :model.artist;
import :model.song;
import :model.dynamic;
import :model.rel_album_artist;
import :model.rel_song_artist;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace q = liorm::query;

export namespace qcm::backend
{
class AlbumFilterExpressions : public FilterExpressions {
public:
    using FilterExpressions::FilterExpressions;
    template<class AlbumTable, class DynamicTable>
    auto build(q::BoundSchema schema, const AlbumTable& album, const DynamicTable& dynamic,
               const QueryFilters<AlbumFilter>& filters) -> Option<q::Expr> {
        for (const auto& filter : filters.values) {
            Option<q::Expr> expr;
            RSTD_MATCH(filter.value) {
                RSTD_CASE(Name, f) {
                    expr =
                        string(album.template col<&Album::name>(), f.value.as_str(), f.condition);
                }
                RSTD_CASE(TrackCount, f) {
                    expr = integer(album.template col<&Album::track_count>(), f.value, f.condition);
                }
                RSTD_CASE(ArtistName, f) {
                    auto song     = schema.template table<Song>();
                    auto relation = schema.template table<RelSongArtist>();
                    auto artist   = schema.template table<Artist>();
                    auto name =
                        string(artist.template col<&Artist::name>(), f.value.as_str(), f.condition);
                    if (name)
                        expr = Some(builder_.exists(
                            builder_.select(builder_.integer(1))
                                .from(song.source())
                                .join(
                                    relation.source(),
                                    builder_.on(schema.relation(db_relations::rel_song_artist_song),
                                                relation,
                                                song))
                                .join(artist.source(),
                                      builder_.on(
                                          schema.relation(db_relations::rel_song_artist_artist),
                                          relation,
                                          artist))
                                .where(builder_.on(schema.relation(db_relations::song_album),
                                                   song,
                                                   album) &&
                                       *name)));
                }
                RSTD_CASE(ArtistId, id) {
                    auto song     = schema.template table<Song>();
                    auto relation = schema.template table<RelSongArtist>();
                    expr          = Some(builder_.exists(
                        builder_.select(builder_.integer(1))
                            .from(song.source())
                            .join(relation.source(),
                                  builder_.on(schema.relation(db_relations::rel_song_artist_song),
                                              relation,
                                              song))
                            .where(builder_.on(
                                       schema.relation(db_relations::song_album), song, album) &&
                                   (relation.template col<&RelSongArtist::artist_id>() ==
                                    number(id)))));
                }
                RSTD_CASE(AlbumArtistId, id) {
                    auto relation = schema.template table<RelAlbumArtist>();
                    auto artist   = schema.template table<Artist>();
                    expr          = Some(builder_.exists(
                        builder_.select(builder_.integer(1))
                            .from(relation.source())
                            .join(
                                artist.source(),
                                builder_.on(schema.relation(db_relations::rel_album_artist_artist),
                                            relation,
                                            artist))
                            .where(
                                builder_.on(schema.relation(db_relations::rel_album_artist_album),
                                            relation,
                                            album) &&
                                (relation.template col<&RelAlbumArtist::artist_id>() ==
                                 number(id)))));
                }
                RSTD_CASE(Year, f) {
                    expr = year(album.template col<&Album::publish_time>(),
                                static_cast<int>(f.value.to_primitive()),
                                f.condition);
                }
                RSTD_CASE(AddedAt, f) {
                    expr = date(album.template col<&Album::added_at>(), f.value, f.condition);
                }
                RSTD_CASE(Duration, f) {
                    expr = integer(album.template col<&Album::duration>(), f.value, f.condition);
                }
                RSTD_CASE(Type, f) {
                    expr = type(album.template col<&Album::type>(), f.value, f.condition);
                }
                RSTD_CASE(DiscCount, f) {
                    expr = integer(album.template col<&Album::disc_count>(), f.value, f.condition);
                }
                RSTD_CASE(LastPlayedAt, f) {
                    expr = date(
                        dynamic.template col<&Dynamic::last_played_at>(), f.value, f.condition);
                }
            }
            add(filter.group.to_primitive(), expr);
        }
        return grouped(filters.logics);
    }
};
} // namespace qcm::backend
