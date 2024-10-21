#include "qcm_interface/oper/album_oper.h"
#include "qcm_interface/model/album.h"

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

template<>
QCM_INTERFACE_API auto Oper<model::Album>::create_list(usize num) -> OperList<model::Album> {
    return detail::create_list<model::Album>(num);
}

IMPL_OPER_PROPERTY(AlbumOper, ItemId, itemId, id)
IMPL_OPER_PROPERTY(AlbumOper, QString, name, name)
IMPL_OPER_PROPERTY(AlbumOper, QString, picUrl, picUrl)
IMPL_OPER_PROPERTY(AlbumOper, QDateTime, publishTime, publishTime)
IMPL_OPER_PROPERTY(AlbumOper, int, trackCount, trackCount)
} // namespace qcm::oper