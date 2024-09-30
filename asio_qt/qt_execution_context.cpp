#include "qt_execution_context.h"

struct QtExecutionContext::EventRunner : QObject {
    EventRunner(QEvent::Type t): event_type(t) {}
    QEvent::Type event_type;

    auto event(QEvent* event) -> bool override {
        if (event->type() == event_type) {
            auto p = static_cast<QtExecEvent*>(event);
            p->accept();
            p->invoke();
            return true;
        } else {
            return QObject::event(event);
        }
    }
};

QtExecutionContext::QtExecutionContext(QObject* target, QEvent::Type t)
    : m_target(new EventRunner(t)) {
    // move to target thread
    if (target->thread() != m_target->thread()) {
        m_target->moveToThread(target->thread());
    }
}
QtExecutionContext::~QtExecutionContext() { m_target->deleteLater(); }

auto QtExecutionContext::event_type() const -> QEvent::Type {
    return static_cast<EventRunner*>(m_target)->event_type;
}
