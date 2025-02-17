#include "qcm_interface/oper/album_oper.h"
#include "qcm_interface/model/album.h"
#include "qcm_interface/oper/artist_oper.h"
#include "qcm_interface/model/artist.h"
#include "qcm_interface/oper/song_oper.h"
#include "qcm_interface/model/song.h"
#include "qcm_interface/oper/mix_oper.h"
#include "qcm_interface/model/mix.h"
#include "qcm_interface/oper/radio_oper.h"
#include "qcm_interface/model/radio.h"
#include "qcm_interface/oper/program_oper.h"
#include "qcm_interface/model/program.h"
#include "qcm_interface/oper/comment_oper.h"
#include "qcm_interface/model/comment.h"
#include "qcm_interface/oper/query_oper.h"
#include "qcm_interface/model/query_model.h"
#include "core/log.h"

void qcm::oper::empty_deletor(voidp) {}
namespace qcm::oper
{

namespace detail
{
template<typename T>
auto create_list(usize num) -> OperList<T> {
    auto holder = typename OperList<T>::Holder(new std::vector<T>(num), [](voidp p) {
        delete static_cast<std::vector<T>*>(p);
    });
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
template<typename T>
auto create_list_ref(std::vector<T>& vec) -> OperList<T> {
    auto holder = typename OperList<T>::Holder(&vec, empty_deletor);
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

#define IMPL_OPER_PROPERTY_LIST(class_, type, prop, mem)                                   \
    auto class_::mem() -> type { return oper::detail::create_list_ref(this->model->mem); } \
    void class_::set_##mem(type v) {                                                       \
        auto& vec = this->model->mem;                                                      \
        vec.clear();                                                                       \
        vec.insert(vec.end(), v.data(), v.data() + v.size());                              \
    }

#define X(T)                                                                \
    template<>                                                              \
    QCM_INTERFACE_API auto Oper<T>::create_list(usize num) -> OperList<T> { \
        return detail::create_list<T>(num);                                 \
    }

X(model::AlbumRefer)
X(model::Album)
X(model::ArtistRefer)
X(model::Artist)
X(model::Song)
X(model::Mix)
X(model::Radio)
X(model::Program)
X(model::Comment)
X(model::ThirdUser)
X(query::Song)

IMPL_OPER_PROPERTY(AlbumReferOper, ItemId, itemId, id)
IMPL_OPER_PROPERTY(AlbumReferOper, QString, name, name)
IMPL_OPER_PROPERTY(AlbumReferOper, QString, picUrl, picUrl)
IMPL_OPER_PROPERTY(AlbumReferOper, qint64, libraryId, libraryId)

IMPL_OPER_PROPERTY(AlbumOper, ItemId, itemId, id)
IMPL_OPER_PROPERTY(AlbumOper, QString, name, name)
IMPL_OPER_PROPERTY(AlbumOper, QString, picUrl, picUrl)
IMPL_OPER_PROPERTY(AlbumOper, qint64, libraryId, libraryId)
IMPL_OPER_PROPERTY(AlbumOper, QDateTime, publishTime, publishTime)
IMPL_OPER_PROPERTY(AlbumOper, int, trackCount, trackCount)
IMPL_OPER_PROPERTY(AlbumOper, QString, description, description)
IMPL_OPER_PROPERTY(AlbumOper, QString, company, company)
IMPL_OPER_PROPERTY(AlbumOper, QString, type, type)

IMPL_OPER_PROPERTY(ArtistReferOper, ItemId, itemId, id)
IMPL_OPER_PROPERTY(ArtistReferOper, QString, name, name)
IMPL_OPER_PROPERTY(ArtistReferOper, QString, picUrl, picUrl)
IMPL_OPER_PROPERTY(ArtistReferOper, qint64, libraryId, libraryId)

IMPL_OPER_PROPERTY(ArtistOper, ItemId, itemId, id)
IMPL_OPER_PROPERTY(ArtistOper, QString, name, name)
IMPL_OPER_PROPERTY(ArtistOper, QString, picUrl, picUrl)
IMPL_OPER_PROPERTY(ArtistOper, qint64, libraryId, libraryId)
IMPL_OPER_PROPERTY(ArtistOper, QString, description, description)
IMPL_OPER_PROPERTY(ArtistOper, qint32, albumCount, albumCount)
IMPL_OPER_PROPERTY(ArtistOper, qint32, musicCount, musicCount)
IMPL_OPER_PROPERTY(ArtistOper, std::vector<QString>, alias, alias)

IMPL_OPER_PROPERTY(SongOper, ItemId, itemId, id)
IMPL_OPER_PROPERTY(SongOper, QString, name, name)
IMPL_OPER_PROPERTY(SongOper, ItemId, albumId, albumId)
IMPL_OPER_PROPERTY(SongOper, qint64, libraryId, libraryId)
IMPL_OPER_PROPERTY(SongOper, qint32, trackNumber, trackNumber)
IMPL_OPER_PROPERTY(SongOper, QDateTime, duration, duration)
IMPL_OPER_PROPERTY(SongOper, bool, canPlay, canPlay)
IMPL_OPER_PROPERTY(SongOper, QString, coverUrl, coverUrl)
IMPL_OPER_PROPERTY(SongOper, QStringList, tags, tags)
IMPL_OPER_PROPERTY(SongOper, qreal, popularity, popularity)
IMPL_OPER_PROPERTY(SongOper, ItemId, sourceId, sourceId)

IMPL_OPER_PROPERTY(MixOper, ItemId, itemId, id)
IMPL_OPER_PROPERTY(MixOper, QString, name, name)
IMPL_OPER_PROPERTY(MixOper, QString, picUrl, picUrl)
IMPL_OPER_PROPERTY(MixOper, qint64, libraryId, libraryId)
IMPL_OPER_PROPERTY(MixOper, qint32, specialType, specialType)
IMPL_OPER_PROPERTY(MixOper, QString, description, description)
IMPL_OPER_PROPERTY(MixOper, QDateTime, createTime, createTime)
IMPL_OPER_PROPERTY(MixOper, QDateTime, updateTime, updateTime)
IMPL_OPER_PROPERTY(MixOper, qint32, playCount, playCount)
IMPL_OPER_PROPERTY(MixOper, qint32, trackCount, trackCount)
IMPL_OPER_PROPERTY(MixOper, ItemId, userId, userId)
IMPL_OPER_PROPERTY(MixOper, std::vector<QString>, tags, tags)

IMPL_OPER_PROPERTY(DjradioOper, ItemId, itemId, id)
IMPL_OPER_PROPERTY(DjradioOper, QString, name, name)
IMPL_OPER_PROPERTY(DjradioOper, QString, picUrl, picUrl)
IMPL_OPER_PROPERTY(DjradioOper, qint64, libraryId, libraryId)
IMPL_OPER_PROPERTY(DjradioOper, QString, description, description)
IMPL_OPER_PROPERTY(DjradioOper, qint32, programCount, programCount)
IMPL_OPER_PROPERTY(DjradioOper, QDateTime, createTime, createTime)

IMPL_OPER_PROPERTY(ProgramOper, ItemId, itemId, id)
IMPL_OPER_PROPERTY(ProgramOper, QString, name, name)
IMPL_OPER_PROPERTY(ProgramOper, QString, description, description)
IMPL_OPER_PROPERTY(ProgramOper, qint64, libraryId, libraryId)
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

namespace qcm::query
{
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

IMPL_OPER_PROPERTY_COPY(SongOper, oper::AlbumReferOper, album, album)
IMPL_OPER_PROPERTY_LIST(SongOper, oper::OperList<model::ArtistRefer>, artists, artists)

} // namespace qcm::query