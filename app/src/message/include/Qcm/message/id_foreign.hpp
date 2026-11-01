#pragma once

#include "Qcm/message/id.hpp"
#include <QtQml/qqmlregistration.h>
#include <QtQml/QQmlEngine>
#include <QtCore/QVariant>

namespace qcm::msg
{
#define QCM_ID_FOREIGN(Name, Kind, QmlName)  \
    struct Name##IdForeign {                 \
        Q_GADGET                             \
        QML_FOREIGN(qcm::msg::id::Name##Id)  \
        QML_VALUE_TYPE(QmlName)              \
    };                                       \
    struct Name##IdsForeign {                \
        Q_GADGET                             \
        QML_FOREIGN(qcm::msg::id::Name##Ids) \
        QML_VALUE_TYPE(QmlName##s)           \
    };
QCM_ID_TYPES(QCM_ID_FOREIGN)
#undef QCM_ID_FOREIGN

class Ids : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    using QObject::QObject;
#define QCM_ID_FACTORY(Name, Kind, QmlName)                                      \
    Q_INVOKABLE qcm::msg::id::Name##Ids QmlName##s(const QVariantList& values) { \
        return collect<qcm::msg::id::Name##Id>(values);                          \
    }
    QCM_ID_TYPES(QCM_ID_FACTORY)
#undef QCM_ID_FACTORY
    Q_INVOKABLE QVariantList artistItems(const qcm::msg::id::ArtistIds& ids) {
        QVariantList items;
        items.reserve(ids.values().size());
        for (auto value : ids.values()) {
            if (value <= 0) {
                if (auto engine = qmlEngine(this))
                    engine->throwError(QStringLiteral("Invalid artist ID collection"));
                return {};
            }
            items.push_back(QVariant::fromValue(identity::fromValue<id::ArtistId>(value)));
        }
        return items;
    }

private:
    template<identity::DomainId T>
    auto collect(const QVariantList& values) -> typename identity::IdTraits<T>::List {
        if (auto result = identity::collect<T>(values)) return *result;
        if (auto engine = qmlEngine(this))
            engine->throwError(QStringLiteral("Invalid domain ID collection"));
        return {};
    }
};
} // namespace qcm::msg
