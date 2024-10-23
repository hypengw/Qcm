#pragma once

#include <set>
#include <string>

#include <asio/awaitable.hpp>
#include <QDateTime>

#include "qcm_interface/item_id.h"
#include "qcm_interface/model/album.h"
#include "qcm_interface/model/artist.h"
#include "qcm_interface/model/song.h"
#include "asio_qt/qt_executor.h"

namespace qcm::db
{

class ItemSqlBase : public NoCopy {
public:
    using IdPair                               = std::tuple<model::ItemId, model::ItemId>;
    virtual auto get_executor() -> QtExecutor& = 0;
    virtual auto insert(std::span<const model::Album> items,
                        const std::set<std::string>&  on_update) -> asio::awaitable<bool> = 0;
    virtual auto insert(std::span<const model::Artist> items,
                        const std::set<std::string>&   on_update) -> asio::awaitable<bool> = 0;
    virtual auto insert(std::span<const model::Song> items,
                        const std::set<std::string>& on_update) -> asio::awaitable<bool> = 0;

    virtual auto insert_album_artist(std::span<const IdPair>) -> asio::awaitable<bool> = 0;
    virtual auto insert_song_artist(std::span<const IdPair>) -> asio::awaitable<bool>  = 0;
};

} // namespace qcm::db