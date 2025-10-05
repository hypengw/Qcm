#pragma once

#include <QtQml/QQmlEngine>
#include <QtGui/QImage>

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
    void specialImageLoaded(const QString& name, QImage img);
};
} // namespace qcm