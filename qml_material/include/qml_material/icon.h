#pragma once

#include <QtCore/QObject>
#include <QtCore/QVariantMap>
#include <QtGui/QColor>
#include <QtQml/QQmlEngine>

namespace qml_material
{

class IconToken : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QVariantMap codeMap READ codeMap NOTIFY codeMapChanged)
public:
    using QObject::QObject;

    const QVariantMap& codeMap() const;

Q_SIGNALS:
    void codeMapChanged();
};

} // namespace qml_material

