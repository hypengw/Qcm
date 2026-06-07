module;
module qcm;
import :image_provider.response;

namespace qcm
{

auto image_response_count() -> Atomic<i32>& {
    static Atomic<i32> count { 0 };
    return count;
}

QcmImageResponse::QcmImageResponse() { image_response_count().fetch_add(1); }
QcmImageResponse::~QcmImageResponse() { image_response_count().fetch_sub(1); }
auto QcmImageResponse::errorString() const -> QString { return m_error; }
void QcmImageResponse::cancel() {
    auto lock  = std::scoped_lock { m_task_mutex };
    m_canceled = true;
    if (m_task) m_task->abort();
}
void QcmImageResponse::setError(QAnyStringView error) { m_error = error.toString(); }
void QcmImageResponse::set_task(rstd::async::JoinHandle<void> task) {
    auto lock = std::scoped_lock { m_task_mutex };
    m_task.emplace(rstd::move(task));
    if (m_canceled) m_task->abort();
}
void QcmImageResponse::done() { QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection); }
void QcmImageResponse::rc_deleter(QcmImageResponse* p) { p->done(); }

} // namespace qcm
