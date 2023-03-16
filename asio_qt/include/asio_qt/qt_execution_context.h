#pragma once

#include <QObject>
#include <QCoreApplication>
#include <QEvent>
#include <QPointer>

#include <asio/execution_context.hpp>

#include "core/core.h"

class QtExecEvent : public QEvent {
public:
    QtExecEvent(): QEvent(generated_type()) {}

    virtual ~QtExecEvent() = default;

    virtual void invoke() = 0;

    static QEvent::Type generated_type() {
        static int event_type = QEvent::registerEventType();
        return static_cast<QEvent::Type>(event_type);
    }
};

template<class F>
struct QtExecFuncEvent : QtExecEvent {
    QtExecFuncEvent(F f): m_f(std::move(f)) {}

    void invoke() override { m_f(); }

private:
    F m_f;
};

class QtExecutionContext : public asio::execution_context, NoCopy {
public:
    QtExecutionContext(QObject*);
    virtual ~QtExecutionContext();

    template<class F>
    void post(F f) {
        if (m_target) {
            auto event = new QtExecFuncEvent(std::move(f));
            QCoreApplication::postEvent(m_target, event);
        }
    }

    struct filter : QObject {
        auto eventFilter(QObject*, QEvent* event) -> bool override;
    };

private:
    QPointer<QObject> m_target;
    filter            m_filter;
};
