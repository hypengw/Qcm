module;
module qcm;
import :image_provider.response;

namespace qcm
{

auto qr_image_response_count() -> Atomic<i32>& {
    static Atomic<i32> count { 0 };
    return count;
}

QrImageResponse::QrImageResponse() { qr_image_response_count().fetch_add(1); }
QrImageResponse::~QrImageResponse() { qr_image_response_count().fetch_sub(1); }
auto QrImageResponse::errorString() const -> QString { return m_error; }
void QrImageResponse::cancel() {
    auto lock  = std::scoped_lock { m_task_mutex };
    m_canceled = true;
    if (m_task) m_task->abort();
}
void QrImageResponse::setError(QAnyStringView error) { m_error = error.toString(); }
void QrImageResponse::set_task(rstd::async::JoinHandle<void> task) {
    auto lock = std::scoped_lock { m_task_mutex };
    m_task.emplace(rstd::move(task));
    if (m_canceled) m_task->abort();
}
void QrImageResponse::done() { QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection); }
void QrImageResponse::rc_deleter(QrImageResponse* p) { p->done(); }

} // namespace qcm
