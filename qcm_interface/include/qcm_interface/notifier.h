#pragma once

#include <QQmlEngine>
#include "qcm_interface/export.h"
#include "qcm_interface/item_id.h"
#include "qcm_interface/enum.h"

namespace qcm
{

class QCM_INTERFACE_API Notifier : public QObject {
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