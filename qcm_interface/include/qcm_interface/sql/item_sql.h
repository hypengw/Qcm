#pragma once

#include <set>
#include <string>

#include <QDateTime>

#include "qcm_interface/item_id.h"
#include "qcm_interface/model/album.h"
#include "qcm_interface/model/artist.h"
#include "qcm_interface/model/song.h"
#include "qcm_interface/model/mix.h"
#include "qcm_interface/model/radio.h"
#include "qcm_interface/model/program.h"
#include "qcm_interface/model/library.h"
#include "asio_qt/qt_executor.h"
#include "core/asio/task.h"

namespace helper
{
class SqlConnect;
}
namespace qcm::db
{

class ItemSqlBase : public NoCopy {
public:
    using IdPair           = std::tuple<model::ItemId, model::ItemId>;
    using ListParam        = const std::set<std::string>&;
    virtual ~ItemSqlBase() = default;

    virtual auto con() const -> rc<helper::SqlConnect> = 0;
    virtual auto get_executor() -> QtExecutor&         = 0;

    virtual auto library_id(i64 provider_id, const QString& native_id) -> task<i64> = 0;
    virtual auto library_id_list() -> task<std::vector<i64>>                        = 0;
    virtual auto library_list() -> task<std::vector<model::Library>>                = 0;

    virtual auto create_library(model::Library) -> task<model::Library> = 0;
    virtual auto delete_library(i64) -> task<bool>                      = 0;

    virtual auto insert(std::span<const model::Album> items, ListParam columns,
                        ListParam on_update = {}) -> task<bool> = 0;
    virtual auto insert(std::span<const model::Artist> items, ListParam columns,
                        ListParam on_update = {}) -> task<bool> = 0;
    virtual auto insert(std::span<const model::Song> items, ListParam columns,
                        ListParam on_update = {}) -> task<bool> = 0;
    virtual auto insert(std::span<const model::Mix> items, ListParam columns,
                        ListParam on_update = {}) -> task<bool> = 0;
    virtual auto insert(std::span<const model::Radio> items, ListParam columns,
                        ListParam on_update = {}) -> task<bool> = 0;
    virtual auto insert(std::span<const model::Program> items, ListParam columns,
                        ListParam on_update = {}) -> task<bool> = 0;
    virtual auto insert(std::span<const model::Library> items, ListParam columns,
                        ListParam on_update = {}) -> task<bool> = 0;

    struct RelationInsertId {
        i32           libraryid;
        model::ItemId first;
        model::ItemId second;
    };

    virtual auto insert_album_artist(std::span<const RelationInsertId>) -> task<bool>  = 0;
    virtual auto insert_song_artist(std::span<const RelationInsertId>) -> task<bool>   = 0;
    virtual auto insert_radio_program(std::span<const RelationInsertId>) -> task<bool> = 0;

    virtual auto insert_mix_song(i32 lib_id, i32 pos, model::ItemId mix_id,
                                 std::span<const model::ItemId> song_ids) -> task<bool>  = 0;
    virtual auto remove_mix_song(i32 lib_id, model::ItemId mix_id,
                                 std::span<const model::ItemId> song_ids) -> task<bool>  = 0;
    virtual auto refresh_mix_song(i32 lib_id, i32 pos, model::ItemId mix_id,
                                  std::span<const model::ItemId> song_ids) -> task<bool> = 0;

    virtual auto select_mix(const model::ItemId& user_id, qint32 special_type)
        -> task<std::optional<model::Mix>> = 0;

    enum class Table
    {
        ALBUM,
        ARTIST,
        SONG,
        ALBUM_ARTIST,
        SONG_ARTIST,
        PLAYLIST,
        PLAYLIST_SONG,
        RADIO,
        LIBRARY
    };
    virtual auto table_name(Table) const -> QStringView = 0;
};

} // namespace qcm::db