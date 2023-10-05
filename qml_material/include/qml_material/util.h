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

    Q_INVOKABLE QColor transparent(QColor in, float alpha) {
        in.setAlphaF(alpha);
        return in;
    }

    Q_INVOKABLE void closePopup(QObject* obj) {
        do {
            auto meta = obj->metaObject();
            do {
                if(meta->className() == std::string("QQuickPopup")) {
                    QMetaObject::invokeMethod(obj, "close");
                    return;
                }
            }
            while(meta = meta->superClass(), meta);
        } while (obj = obj->parent(), obj);
    }

    Q_INVOKABLE QColor hoverColor(QColor in) {
        in.setAlphaF(0.08);
        return in;
    }

    Q_INVOKABLE QColor pressColor(QColor in) {
        in.setAlphaF(0.18);
        return in;
    }
};
} // namespace qml_material