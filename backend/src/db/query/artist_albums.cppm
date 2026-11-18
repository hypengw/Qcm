export module qcm.backend:db.query.artist_albums;
export import :db.query.input;
export import :db.error.query;

export namespace qcm::backend
{
template<class Database>
auto artist_album_page(Database& db, ArtistAlbumQuery input) -> QueryResult<Option<IdQueryPage>>;

extern template auto artist_album_page(sqlito::Connection&, ArtistAlbumQuery)
    -> QueryResult<Option<IdQueryPage>>;
extern template auto artist_album_page(sqlito::Transaction&, ArtistAlbumQuery)
    -> QueryResult<Option<IdQueryPage>>;
} // namespace qcm::backend
