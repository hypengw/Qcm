#include "Qcm/image_provider/response.hpp"

#include <atomic>

namespace qcm
{

auto image_response_count() -> std::atomic<i32>& {
    static std::atomic<i32> count { 0 };
    return count;
}

QcmImageResponse::QcmImageResponse() { image_response_count()++; }
QcmImageResponse::~QcmImageResponse() { image_response_count()--; }
auto QcmImageResponse::errorString() const -> QString { return m_error; }
void QcmImageResponse::set_error(QAnyStringView error) { m_error = error.toString(); }
void QcmImageResponse::done() { QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection); }
void QcmImageResponse::rc_deleter(QcmImageResponse* p) { p->done(); }

} // namespace qcm