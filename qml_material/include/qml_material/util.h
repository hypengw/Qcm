#pragma once
#include <QObject>
#include <QQmlEngine>
#include <QColor>

namespace qml_material
{
class Util : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    using QObject::QObject;

    Q_INVOKABLE bool hasIcon(const QJSValue& v) {
        auto name   = v.property("name");
        auto source = v.property("source");
        if (name.isString() && source.isVariant()) {
            return ! name.toString().isEmpty() || ! source.toString().isEmpty();
        }
        return false;
    }
};
} // namespace qml_material