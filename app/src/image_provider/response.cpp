module;
module qcm;
import :image_provider.response;

namespace qcm
{

auto image_response_count() -> Atomic<i32>& {
    static Atomic<i32> count { 0 };
    return count;
}

QcmImageResponse::QcmImageResponse() { image_response_count()++; }
QcmImageResponse::~QcmImageResponse() { image_response_count()--; }
auto QcmImageResponse::errorString() const -> QString { return m_error; }
void QcmImageResponse::setError(QAnyStringView error) { m_error = error.toString(); }
void QcmImageResponse::done() { QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection); }
void QcmImageResponse::rc_deleter(QcmImageResponse* p) { p->done(); }

} // namespace qcm