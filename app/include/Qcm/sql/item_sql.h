#pragma once

#include "core/core.h"
#include "qcm_interface/sql/item_sql.h"

namespace helper
{
class SqlConnect;
}
namespace qcm
{

class ItemSql : public db::ItemSqlBase {
public:
    ItemSql(rc<helper::SqlConnect> con);
    ~ItemSql();

    auto get_executor() -> QtExecutor& override;
    auto con() const -> rc<helper::SqlConnect> override;

    auto insert(std::span<const model::Album> items, ListParam columns,
                ListParam on_update = {}) -> task<bool> override;
    auto insert(std::span<const model::Artist> items, ListParam columns,
                ListParam on_update = {}) -> task<bool> override;
    auto insert(std::span<const model::Song> items, ListParam columns,
                ListParam on_update = {}) -> task<bool> override;
    auto insert(std::span<const model::Playlist> items, ListParam columns,
                ListParam on_update = {}) -> task<bool> override;
    auto insert(std::span<const model::Radio> items, ListParam columns,
                ListParam on_update = {}) -> task<bool> override;
    auto insert(std::span<const model::Program> items, ListParam columns,
                ListParam on_update = {}) -> task<bool> override;

    auto insert_album_artist(std::span<const IdPair>) -> task<bool> override;
    auto insert_song_artist(std::span<const IdPair>) -> task<bool> override;
    auto insert_radio_program(std::span<const IdPair>) -> task<bool> override;
    auto insert_playlist_song(i32 pos, model::ItemId palylist_id,
                              std::span<const model::ItemId> song_ids) -> task<bool> override;
    auto table_name(Table) const -> QStringView override;
    auto clean(const QDateTime& before, Table table) -> task<void>;
    auto missing(std::span<const model::ItemId> ids, Table table, std::optional<Table> join = {},
                 ListParam not_null = {}) -> task<std::vector<model::ItemId>>;

private:
    void create_album_table();
    void create_artist_table();
    void create_album_artist_table();
    void create_song_table();
    void create_song_artist_table();
    void create_playlist_table();
    void create_playlist_song_table();
    void create_radio_table();
    void create_program_table();
    void create_radio_program_table();

    QString                m_album_table;
    QString                m_artist_table;
    QString                m_song_table;
    QString                m_album_artist_table;
    QString                m_song_artist_table;
    QString                m_playlist_table;
    QString                m_playlist_song_table;
    QString                m_radio_table;
    QString                m_program_table;
    QString                m_radio_program_table;
    rc<helper::SqlConnect> m_con;
};

} // namespace qcm
