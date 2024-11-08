#include "qcm_interface/oper/album_oper.h"
#include "qcm_interface/model/album.h"
#include "qcm_interface/oper/artist_oper.h"
#include "qcm_interface/model/artist.h"
#include "qcm_interface/oper/song_oper.h"
#include "qcm_interface/model/song.h"
#include "qcm_interface/oper/playlist_oper.h"
#include "qcm_interface/model/playlist.h"
#include "qcm_interface/oper/radio_oper.h"
#include "qcm_interface/model/radio.h"
#include "qcm_interface/oper/program_oper.h"
#include "qcm_interface/model/program.h"
#include "qcm_interface/oper/comment_oper.h"
#include "qcm_interface/model/comment.h"
#include "core/log.h"

void qcm::oper::empty_deletor(voidp p) { _assert_rel_(p == nullptr); }
namespace qcm::oper
{

namespace detail
{
template<typename T>
auto create_list(usize num) -> OperList<T> {
    auto holder = typename OperList<T>::Holder(new std::vector<T>(num), [](voidp p) {
        delete static_cast<std::vector<T>*>(p);
    });
    auto vec    = static_cast<std::vector<T>*>(holder.get());
    auto span   = std::span { *vec };
    return { std::move(holder),
             [](voidp h) {
                 return static_cast<std::vector<T>*>(h)->data();
             },
             [](voidp h) {
                 return static_cast<std::vector<T>*>(h)->size();
             },
             [](T* d, usize s) -> T* {
                 return d + s;
             },
             [](voidp handle) -> T* {
                 return &(static_cast<std::vector<T>*>(handle)->emplace_back());
             } };
}
} // namespace detail

#define X(T)                                                                \
    template<>                                                              \
    QCM_INTERFACE_API auto Oper<T>::create_list(usize num) -> OperList<T> { \
        return detail::create_list<T>(num);                                 \
    }

X(model::Album)
X(model::Artist)
X(model::Song)
X(model::Playlist)
X(model::Radio)
X(model::Program)
X(model::Comment)
X(model::ThirdUser)

IMPL_OPER_PROPERTY(AlbumOper, ItemId, itemId, id)
IMPL_OPER_PROPERTY(AlbumOper, QString, name, name)
IMPL_OPER_PROPERTY(AlbumOper, QString, picUrl, picUrl)
IMPL_OPER_PROPERTY(AlbumOper, QDateTime, publishTime, publishTime)
IMPL_OPER_PROPERTY(AlbumOper, int, trackCount, trackCount)
IMPL_OPER_PROPERTY(AlbumOper, QString, description, description)
IMPL_OPER_PROPERTY(AlbumOper, QString, company, company)
IMPL_OPER_PROPERTY(AlbumOper, QString, type, type)

IMPL_OPER_PROPERTY(ArtistOper, ItemId, itemId, id)
IMPL_OPER_PROPERTY(ArtistOper, QString, name, name)
IMPL_OPER_PROPERTY(ArtistOper, QString, picUrl, picUrl)
IMPL_OPER_PROPERTY(ArtistOper, QString, description, description)
IMPL_OPER_PROPERTY(ArtistOper, qint32, albumCount, albumCount)
IMPL_OPER_PROPERTY(ArtistOper, qint32, musicCount, musicCount)
IMPL_OPER_PROPERTY(ArtistOper, std::vector<QString>, alias, alias)

IMPL_OPER_PROPERTY(SongOper, ItemId, itemId, id)
IMPL_OPER_PROPERTY(SongOper, QString, name, name)
IMPL_OPER_PROPERTY(SongOper, ItemId, albumId, albumId)
IMPL_OPER_PROPERTY(SongOper, qint32, trackNumber, trackNumber)
IMPL_OPER_PROPERTY(SongOper, QDateTime, duration, duration)
IMPL_OPER_PROPERTY(SongOper, bool, canPlay, canPlay)
IMPL_OPER_PROPERTY(SongOper, QString, coverUrl, coverUrl)
IMPL_OPER_PROPERTY(SongOper, QStringList, tags, tags)
IMPL_OPER_PROPERTY(SongOper, qreal, popularity, popularity)
IMPL_OPER_PROPERTY(SongOper, ItemId, sourceId, sourceId)

IMPL_OPER_PROPERTY(PlaylistOper, ItemId, itemId, id)
IMPL_OPER_PROPERTY(PlaylistOper, QString, name, name)
IMPL_OPER_PROPERTY(PlaylistOper, QString, picUrl, picUrl)
IMPL_OPER_PROPERTY(PlaylistOper, QString, description, description)
IMPL_OPER_PROPERTY(PlaylistOper, QDateTime, createTime, createTime)
IMPL_OPER_PROPERTY(PlaylistOper, QDateTime, updateTime, updateTime)
IMPL_OPER_PROPERTY(PlaylistOper, qint32, playCount, playCount)
IMPL_OPER_PROPERTY(PlaylistOper, qint32, trackCount, trackCount)
IMPL_OPER_PROPERTY(PlaylistOper, ItemId, userId, userId)
IMPL_OPER_PROPERTY(PlaylistOper, std::vector<QString>, tags, tags)

IMPL_OPER_PROPERTY(DjradioOper, ItemId, itemId, id)
IMPL_OPER_PROPERTY(DjradioOper, QString, name, name)
IMPL_OPER_PROPERTY(DjradioOper, QString, picUrl, picUrl)
IMPL_OPER_PROPERTY(DjradioOper, QString, description, description)
IMPL_OPER_PROPERTY(DjradioOper, qint32, programCount, programCount)
IMPL_OPER_PROPERTY(DjradioOper, QDateTime, createTime, createTime)

IMPL_OPER_PROPERTY(ProgramOper, ItemId, itemId, id)
IMPL_OPER_PROPERTY(ProgramOper, QString, name, name)
IMPL_OPER_PROPERTY(ProgramOper, QString, description, description)
IMPL_OPER_PROPERTY(ProgramOper, QDateTime, duration, duration)
IMPL_OPER_PROPERTY(ProgramOper, QString, coverUrl, coverUrl)
IMPL_OPER_PROPERTY(ProgramOper, ItemId, songId, songId)
IMPL_OPER_PROPERTY(ProgramOper, QDateTime, createTime, createTime)
IMPL_OPER_PROPERTY(ProgramOper, qint32, serialNumber, serialNumber)
IMPL_OPER_PROPERTY(ProgramOper, ItemId, radioId, radioId)

IMPL_OPER_PROPERTY(CommentOper, ItemId, itemId, id)
IMPL_OPER_PROPERTY(CommentOper, QString, content, content)
IMPL_OPER_PROPERTY_COPY(CommentOper, ThirdUserOper, user, user)
IMPL_OPER_PROPERTY(CommentOper, QDateTime, time, time)
IMPL_OPER_PROPERTY(CommentOper, bool, liked, liked)

IMPL_OPER_PROPERTY(ThirdUserOper, ItemId, itemId, id)
IMPL_OPER_PROPERTY(ThirdUserOper, QString, name, name)
IMPL_OPER_PROPERTY(ThirdUserOper, QString, picUrl, picUrl)
} // namespace qcm::oper