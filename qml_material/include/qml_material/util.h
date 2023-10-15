#pragma once
#include <QObject>
#include <QQmlEngine>
#include <QColor>
#include <QQuickItem>
#include <QPointer>

namespace qml_material
{

class InputBlock : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool when READ when WRITE setWhen NOTIFY whenChanged)
    Q_PROPERTY(QQuickItem* target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(Qt::MouseButtons acceptMouseButtons READ acceptMouseButtons WRITE setAcceptMouseButtons NOTIFY acceptMouseButtonsChanged)
    Q_PROPERTY(bool acceptHover READ acceptHoverEvents WRITE setAcceptHoverEvents NOTIFY acceptHoverEventsChanged)
    Q_PROPERTY(bool acceptTouch READ acceptTouchEvents WRITE setAcceptTouchEvents NOTIFY acceptTouchEventsChanged)

public:
    InputBlock(QObject* parent = nullptr);

    QQuickItem* target() const;
    void        setTarget(QQuickItem*);

    bool when() const;
    void setWhen(bool);

    Qt::MouseButtons acceptMouseButtons() const;
    void setAcceptMouseButtons(Qt::MouseButtons buttons);
    bool acceptHoverEvents() const;
    void setAcceptHoverEvents(bool enabled);
    bool acceptTouchEvents() const;
    void setAcceptTouchEvents(bool accept);

signals:
    void whenChanged();
    void targetChanged();
    void acceptMouseButtonsChanged();
    void acceptHoverEventsChanged();
    void acceptTouchEventsChanged();

public slots:
    void trigger();

private:
    struct State {
        bool             canHover { false };
        bool             canTouch { false };
        Qt::MouseButtons mouseButtons { Qt::NoButton };
        void             saveState(QQuickItem*);
        void             restoreState(QQuickItem*);
    };

private:
    bool                 mWhen;
    QPointer<QQuickItem> mTarget;
    State                mState;
    State                mReqState;
};

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
};
} // namespace qml_material