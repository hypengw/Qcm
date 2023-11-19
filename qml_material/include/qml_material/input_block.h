#pragma once

#include <QQmlEngine>
#include <QQuickItem>

namespace qml_material
{

class InputBlock : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool when READ when WRITE setWhen NOTIFY whenChanged)
    Q_PROPERTY(QQuickItem* target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(Qt::MouseButtons acceptMouseButtons READ acceptMouseButtons WRITE
                   setAcceptMouseButtons NOTIFY acceptMouseButtonsChanged)
    Q_PROPERTY(bool acceptHover READ acceptHoverEvents WRITE setAcceptHoverEvents NOTIFY
                   acceptHoverEventsChanged)
    Q_PROPERTY(bool acceptTouch READ acceptTouchEvents WRITE setAcceptTouchEvents NOTIFY
                   acceptTouchEventsChanged)

public:
    InputBlock(QObject* parent = nullptr);

    QQuickItem* target() const;
    void        setTarget(QQuickItem*);

    bool when() const;
    void setWhen(bool);

    Qt::MouseButtons acceptMouseButtons() const;
    void             setAcceptMouseButtons(Qt::MouseButtons buttons);
    bool             acceptHoverEvents() const;
    void             setAcceptHoverEvents(bool enabled);
    bool             acceptTouchEvents() const;
    void             setAcceptTouchEvents(bool accept);

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

} // namespace qml_material