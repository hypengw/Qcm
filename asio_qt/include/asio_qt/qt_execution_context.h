#pragma once

#include <QObject>
#include <QCoreApplication>
#include <QEvent>
#include <QPointer>

#include <asio/execution_context.hpp>

#include "core/core.h"

class QtExecEvent : public QEvent {
public:
    QtExecEvent(QEvent::Type t): QEvent(t) {}

    virtual ~QtExecEvent() = default;

    virtual void invoke() = 0;
};

template<class F>
struct QtExecFuncEvent : QtExecEvent {
    QtExecFuncEvent(F&& f, QEvent::Type t): QtExecEvent(t), m_f(std::forward<F>(f)) {}

    void invoke() override { m_f(); }

private:
    F m_f;
};

class QtExecutionContext : public asio::execution_context, NoCopy {
public:
    QtExecutionContext(QObject*, QEvent::Type);
    QtExecutionContext(QThread*, QEvent::Type);
    virtual ~QtExecutionContext();

    template<class F>
    void post(F&& f) {
        auto event = new QtExecFuncEvent(std::forward<F>(f), event_type());
        QCoreApplication::postEvent(m_target, event);
    }

    auto event_type() const -> QEvent::Type;

private:
    struct EventRunner;
    QObject* m_target;
};
