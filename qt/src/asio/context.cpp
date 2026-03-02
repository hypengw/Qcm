module qcm.qt;
import :asio.context;
import :asio.executor;

QtExecutionContext::QtExecutionContext(QThread* thread, QEvent::Type t)
    : m_target(new QtExecutionEventRunner(t)),
      m_ex(Box<QtExecutor>::from_raw(
          rstd::mut_ptr<QtExecutor>::from_raw_parts(new QtExecutor { this }))) {
    // move to target thread
    if (thread != m_target->thread()) {
        m_target->moveToThread(thread);
    }
}
QtExecutionContext::QtExecutionContext(QObject* target, QEvent::Type t)
    : QtExecutionContext(target->thread(), t) {}

QtExecutionContext::~QtExecutionContext() { m_target->deleteLater(); }

auto QtExecutionContext::event_type() const -> QEvent::Type { return m_target->event_type; }

auto QtExecutionContext::get_executor() -> QtExecutor& { return *m_ex.as_mut_ptr(); }

auto QtExecutionEventRunner::event(QEvent* event) -> bool {
    if (event->type() == event_type) {
        auto p = static_cast<QtExecEvent*>(event);
        p->accept();
        p->invoke();
        return true;
    } else {
        return QObject::event(event);
    }
}
