#include "qt_execution_context.h"

struct QtExecutionContext::Filter : QObject {
    Filter(QEvent::Type t): event_type(t) {}
    QEvent::Type event_type;

    auto eventFilter(QObject*, QEvent* event) -> bool override;
};

QtExecutionContext::QtExecutionContext(QObject* target, QEvent::Type t)
    : m_target(target), m_filter(new Filter(t)) {
    m_target->installEventFilter(m_filter);
}
QtExecutionContext::~QtExecutionContext() {
    if (m_target) {
        m_target->removeEventFilter(m_filter);
    }
    delete m_filter;
}

auto QtExecutionContext::event_type() const -> QEvent::Type {
    return m_filter->event_type;
}

auto QtExecutionContext::Filter::eventFilter(QObject*, QEvent* event) -> bool {
    if (event->type() == event_type) {
        auto p = static_cast<QtExecEvent*>(event);
        p->accept();
        p->invoke();
        return true;
    } else
        return false;
}
