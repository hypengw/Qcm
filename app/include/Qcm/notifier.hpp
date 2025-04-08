#pragma once

#include <QQmlEngine>
#include "core/core.h"
#include "Qcm/model/item_id.hpp"
#include "Qcm/qml/enum.hpp"

namespace qcm
{

class Notifier : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    Notifier(QObject* parent);
    ~Notifier();
    static auto      instance() -> Notifier*;
    static Notifier* create(QQmlEngine*, QJSEngine*);
    // make qml prefer create
    Notifier() = delete;

Q_SIGNALS:
    void collected(const model::ItemId&, bool);
    void collection_synced(enums::CollectionType type, model::ItemId userId, QDateTime time);
    void itemChanged(const model::ItemId&);
};
} // namespace qcm