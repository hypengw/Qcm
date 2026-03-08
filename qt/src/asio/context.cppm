export module qcm.qt:asio.context;
export import :asio.forward;
export import qcm.core;
export import qcm.asio;
export import qt;

using rstd::boxed::Box;

class QtExecEvent : public QEvent {
public:
    QtExecEvent(QEvent::Type t): QEvent(t) {}

    virtual ~QtExecEvent() = default;

    virtual void invoke() = 0;
};

template<class F>
struct QtExecFuncEvent : QtExecEvent {
    QtExecFuncEvent(F&& f, QEvent::Type t): QtExecEvent(t), m_f(rstd::forward<F>(f)) {}

    void invoke() override { m_f(); }

private:
    F m_f;
};

struct QtExecutionEventRunner : QObject {
    QtExecutionEventRunner(QEvent::Type t): event_type(t) {}
    QEvent::Type event_type;
    auto         event(QEvent* event) -> bool override;
};

class QtExecutor;

export class QtExecutionContext : public asio::execution_context, NoCopy {
public:
    QtExecutionContext(QObject*, QEvent::Type);
    QtExecutionContext(QThread*, QEvent::Type);
    virtual ~QtExecutionContext();

    QtExecutionContext(const QtExecutionContext&) = delete;
    QtExecutionContext(QtExecutionContext&&)      = delete;

    auto get_executor() -> QtExecutor&;

    template<class F>
    void post(F&& f) {
        auto event = new QtExecFuncEvent(rstd::forward<F>(f), event_type());
        QCoreApplication::postEvent(m_target, event);
    }

    auto event_type() const -> QEvent::Type;

private:
    QtExecutionEventRunner* m_target;
    Box<QtExecutor>   m_ex;
};
