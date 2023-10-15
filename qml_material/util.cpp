#include "qml_material/util.h"

using namespace qml_material;

InputBlock::InputBlock(QObject* parent): QObject(parent), mWhen(false), mTarget(nullptr) {
    connect(this, &InputBlock::whenChanged, this, &InputBlock::trigger, Qt::QueuedConnection);
    connect(this, &InputBlock::targetChanged, this, &InputBlock::trigger, Qt::QueuedConnection);
    connect(this, &InputBlock::acceptMouseButtonsChanged, this, &InputBlock::trigger, Qt::QueuedConnection);
    connect(this, &InputBlock::acceptHoverEventsChanged, this, &InputBlock::trigger, Qt::QueuedConnection);
    connect(this, &InputBlock::acceptTouchEventsChanged, this, &InputBlock::trigger, Qt::QueuedConnection);
}

bool InputBlock::when() const { return mWhen; }
void InputBlock::setWhen(bool v) {
    if (std::exchange(mWhen, v) != v) {
        whenChanged();
    }
}

QQuickItem* InputBlock::target() const { return mTarget; }
void        InputBlock::setTarget(QQuickItem* v) {
    if (auto old = std::exchange(mTarget, v); old != v) {
        if (old) {
            mState.restoreState(old);
        }
        if (mTarget) {
            mState.saveState(mTarget);
        }
        targetChanged();
    }
}

void InputBlock::State::saveState(QQuickItem* target) {
    canHover     = target->acceptHoverEvents();
    canTouch     = target->acceptTouchEvents();
    mouseButtons = target->acceptedMouseButtons();
}

void InputBlock::State::restoreState(QQuickItem* target) {
    target->setAcceptedMouseButtons(mouseButtons);
    target->setAcceptTouchEvents(canTouch);
    target->setAcceptHoverEvents(canHover);
}

void InputBlock::trigger() {
    if (mTarget) {
        if (mWhen) {
            mReqState.restoreState(mTarget);
        } else {
            mState.restoreState(mTarget);
        }
    }
}

Qt::MouseButtons InputBlock::acceptMouseButtons() const { return mReqState.mouseButtons; }
void             InputBlock::setAcceptMouseButtons(Qt::MouseButtons buttons) {
    if (std::exchange(mReqState.mouseButtons, buttons)) {
        acceptMouseButtonsChanged();
    }
}
bool InputBlock::acceptHoverEvents() const { return mReqState.canHover; }
void InputBlock::setAcceptHoverEvents(bool enabled) {
    if (std::exchange(mReqState.canHover, enabled)) {
        acceptHoverEventsChanged();
    }
}
bool InputBlock::acceptTouchEvents() const { return mReqState.canTouch; }
void InputBlock::setAcceptTouchEvents(bool accept) {
    if (std::exchange(mReqState.canTouch, accept)) {
        acceptTouchEventsChanged();
    }
}