#include "qt_execution_context.h"

QtExecutionContext::QtExecutionContext(QObject* target): m_target(target) {
    m_target->installEventFilter(&m_filter);
}
QtExecutionContext::~QtExecutionContext() {
    if (m_target) {
        m_target->removeEventFilter(&m_filter);
    }
}

auto QtExecutionContext::filter::eventFilter(QObject*, QEvent* event) -> bool {
    if (event->type() == QtExecEvent::generated_type()) {
        auto p = static_cast<QtExecEvent*>(event);
        p->accept();
        p->invoke();
        return true;
    } else
        return false;
}
