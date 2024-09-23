#pragma once

#include <QQmlEngine>

#include "qcm_interface/model/page.h"

namespace qcm::qml
{

class Util : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    Q_INVOKABLE model::Page create_page() const;
    Q_INVOKABLE model::ItemId create_itemid() const;
    Q_INVOKABLE QString mpris_trackid(model::ItemId) const;
};

} // namespace qcm::qml