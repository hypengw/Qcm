#pragma once

#include <set>
#include <string>

#include <QDateTime>

#include "qcm_interface/item_id.h"
#include "qcm_interface/model/album.h"
#include "qcm_interface/model/artist.h"
#include "qcm_interface/model/song.h"
#include "qcm_interface/model/playlist.h"
#include "asio_qt/qt_executor.h"
#include "asio_helper/task.h"

namespace qcm::db
{

class ItemSqlBase : public NoCopy {
public:
    using IdPair = std::tuple<model::ItemId, model::ItemId>;

    virtual auto get_executor() -> QtExecutor&                                = 0;
    virtual auto insert(std::span<const model::Album> items,
                        const std::set<std::string>&  on_update) -> task<bool> = 0;
    virtual auto insert(std::span<const model::Artist> items,
                        const std::set<std::string>&   on_update) -> task<bool> = 0;
    virtual auto insert(std::span<const model::Song> items,
                        const std::set<std::string>& on_update) -> task<bool> = 0;
    virtual auto insert(std::span<const model::Playlist> items,
                        const std::set<std::string>&     on_update) -> task<bool> = 0;

    virtual auto insert_album_artist(std::span<const IdPair>) -> task<bool>                  = 0;
    virtual auto insert_song_artist(std::span<const IdPair>) -> task<bool>                   = 0;
    virtual auto insert_playlist_song(i32 pos, model::ItemId palylist_id,
                                      std::span<const model::ItemId> song_ids) -> task<bool> = 0;

    enum class Table
    {
        ALBUM,
        ARTIST,
        SONG,
        ALBUM_ARTIST,
        SONG_ARTIST,
        PLAYLIST,
        PLAYLIST_SONG,
    };
    virtual auto table_name(Table) const -> QStringView = 0;
};

} // namespace qcm::db