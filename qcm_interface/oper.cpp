#include "qcm_interface/oper/album_oper.h"
#include "qcm_interface/model/album.h"
#include "qcm_interface/oper/artist_oper.h"
#include "qcm_interface/model/artist.h"

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
    return { std::move(holder), span.data(), span.size(), [](T* d, usize s) -> T* {
                return d + s;
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

IMPL_OPER_PROPERTY(AlbumOper, ItemId, itemId, id)
IMPL_OPER_PROPERTY(AlbumOper, QString, name, name)
IMPL_OPER_PROPERTY(AlbumOper, QString, picUrl, picUrl)
IMPL_OPER_PROPERTY(AlbumOper, QDateTime, publishTime, publishTime)
IMPL_OPER_PROPERTY(AlbumOper, int, trackCount, trackCount)

IMPL_OPER_PROPERTY(ArtistOper, ItemId, itemId, id)
IMPL_OPER_PROPERTY(ArtistOper, QString, name, name)
IMPL_OPER_PROPERTY(ArtistOper, QString, picUrl, picUrl)
IMPL_OPER_PROPERTY(ArtistOper, QString, briefDesc, briefDesc)
IMPL_OPER_PROPERTY(ArtistOper, qint32, albumCount, albumCount)
IMPL_OPER_PROPERTY(ArtistOper, qint32, musicCount, musicCount)
IMPL_OPER_PROPERTY(ArtistOper, std::vector<QString>, alias, alias)
} // namespace qcm::oper