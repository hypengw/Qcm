#pragma once

#include "Qcm/message/id.qpb.h"
#include "Qcm/message/id_types.hpp"
#include "Qcm/message/item_type.hpp"
#include <concepts>
#include <QtCore/QVariant>
#include <optional>

namespace qcm::identity
{
template<class T>
struct IdTraits;
template<class T>
struct IdListTraits;

#define QCM_ID_TRAITS(Name, Kind, QmlName)                  \
    template<>                                              \
    struct IdTraits<msg::id::Name##Id> {                    \
        static constexpr auto type = enums::ItemType::Kind; \
        using List                 = msg::id::Name##Ids;    \
    };                                                      \
    template<>                                              \
    struct IdListTraits<msg::id::Name##Ids> {               \
        using Id = msg::id::Name##Id;                       \
    };
QCM_ID_TYPES(QCM_ID_TRAITS)
#undef QCM_ID_TRAITS

template<class T>
concept DomainId = requires { IdTraits<T>::type; };
template<class T>
concept DomainIds = requires { typename IdListTraits<T>::Id; };

template<DomainId T>
auto valid(const T& id) -> bool {
    return id.value() > 0;
}
template<DomainIds T>
auto valid(const T& ids) -> bool {
    for (auto value : ids.values())
        if (value <= 0) return false;
    return true;
}

template<DomainId T>
auto fromValue(qint64 value) -> T {
    T id;
    id.setValue(value);
    return id;
}
template<DomainIds T, class Range>
auto fromValues(const Range& values) -> T {
    T ids;
    ids.setValues({ values.begin(), values.end() });
    return ids;
}

template<DomainId T>
auto collect(const QVariantList& values) -> std::optional<typename IdTraits<T>::List> {
    typename IdTraits<T>::List result;
    QtProtobuf::int64List      ids;
    ids.reserve(values.size());
    for (auto value : values) {
        if (! value.convert(QMetaType::fromType<T>()) || ! valid(value.value<T>())) return {};
        ids.push_back(value.value<T>().value());
    }
    result.setValues(ids);
    return result;
}
} // namespace qcm::identity
