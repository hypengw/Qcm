#pragma once
#include <QObject>
#include <QQmlEngine>
#include <QColor>
#include <QQuickItem>
#include <QPointer>
#include <QQuickWindow>
#include <QtDBus/QDBusVariant>

#include "qml_material/corner.h"
#include "qml_material/type.h"
#include "core/core.h"

namespace qml_material
{

class Xdp : public QObject {
    Q_OBJECT
public:

    Xdp(QObject* parent = nullptr);
    ~Xdp();

    static Xdp* insance();

    QColor accentColor() const;
    Qt::ColorScheme colorScheme() const;
public Q_SLOTS:
    void xdpSettingChangeSlot(QString, QString, QDBusVariant);

Q_SIGNALS:
    void colorSchemeChanged();
    void accentColorChanged();

private:
    std::optional<Qt::ColorScheme> m_color_scheme;
    std::optional<QColor> m_accent_color;
};

class Util : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    Util(QObject* parent = nullptr);
    ~Util();

    enum Track
    {
        TrackCreate = 0,
        TrackDelete
    };
    Q_ENUMS(Track)

    Q_INVOKABLE void track(QVariant, Track);

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
                if (meta->className() == std::string("QQuickPopup")) {
                    QMetaObject::invokeMethod(obj, "close");
                    return;
                }
            } while (meta = meta->superClass(), meta);
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

    Q_INVOKABLE qreal devicePixelRatio(QQuickItem* in) {
        return in ? in->window() ? in->window()->devicePixelRatio() : 1.0 : 1.0;
    }

    // tl tr bl br
    Q_INVOKABLE CornersGroup corner(QVariant in);

    Q_INVOKABLE CornersGroup corner(qreal br, qreal tr, qreal bl, qreal tl);

    QString type_str(const QJSValue&);
    Q_INVOKABLE void print_parents(const QJSValue&);
private:
    usize m_tracked { 0 };
};
} // namespace qml_material