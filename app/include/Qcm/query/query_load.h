#pragma once
#include <QSqlQuery>
#include "qcm_interface/model/query_model.h"
#include "qcm_interface/sql/meta_sql.h"
namespace qcm::query
{
using Converter = std::function<QVariant(const QVariant&)>;
auto get_from_converter(int id) -> std::optional<Converter>;
auto get_to_converter(int id) -> std::optional<Converter>;

template<typename T>
concept LibraryItemCP = requires(T t) {
    { t.libraryId } -> std::same_as<i64>;
    { t.id } -> std::same_as<model::ItemId>;
};

template<typename T>
void load_query(QSqlQuery& query, T& gad, int& i) {
    for (auto prop_i : T::sql().idxs) {
        auto val  = query.value(i++);
        auto prop = T::staticMetaObject.property(prop_i);
        if (auto c = get_from_converter(prop.metaType().id())) {
            val = (*c)(val);
        }
        prop.writeOnGadget(&gad, val);

        if constexpr (LibraryItemCP<T>) {
            gad.id.set_library_id(gad.libraryId);
        }

        if constexpr (std::is_base_of_v<model::AlbumRefer, T>) {
            gad.id.set_type("album");
        } else if constexpr (std::is_base_of_v<model::ArtistRefer, T>) {
            gad.id.set_type("artist");
        } else if constexpr (std::is_base_of_v<model::MixRefer, T>) {
            gad.id.set_type("mix");
        } else if constexpr (std::is_base_of_v<model::Song, T>) {
            gad.id.set_type("song");
        }
    }
}

template<>
inline void load_query<std::vector<model::ArtistRefer>>(QSqlQuery&                       query,
                                                        std::vector<model::ArtistRefer>& artists,
                                                        int&                             i) {
    auto artist_ids     = query.value(i++).toString().split(',');
    auto artist_names   = query.value(i++).toString().split(',');
    auto artist_picUrls = query.value(i++).toString().split(',');

    auto num = artist_ids.size();
    for (qsizetype i = 0; i < num; i++) {
        auto& ar = artists.emplace_back();
        ar.id    = artist_ids[i];
        if (i < artist_names.size()) ar.name = artist_names[i];
        if (i < artist_picUrls.size()) ar.picUrl = artist_picUrls[i];
    }
}

template<>
inline void load_query<Album>(QSqlQuery& query, Album& gad, int& i) {
    load_query<model::Album>(query, gad, i);
    load_query(query, gad.artists, i);
}
template<>
inline void load_query<Song>(QSqlQuery& query, Song& gad, int& i) {
    load_query<model::Song>(query, gad, i);
    load_query<model::AlbumRefer>(query, gad.album, i);
    load_query(query, gad.artists, i);
}

} // namespace qcm::query