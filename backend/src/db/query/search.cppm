export module qcm.backend:db.query.search;
export import :db.query.input;
export import :db.error.query;
export import sqlito;

export namespace qcm::backend
{
template<class Database>
auto search_page(Database& db, SearchQuery input) -> QueryResult<IdQueryPage>;

extern template auto search_page(sqlito::Connection&, SearchQuery) -> QueryResult<IdQueryPage>;
extern template auto search_page(sqlito::Transaction&, SearchQuery) -> QueryResult<IdQueryPage>;
} // namespace qcm::backend
