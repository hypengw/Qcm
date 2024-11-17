#pragma once

#include <QtQuick/QQuickImageResponse>
#include "core/core.h"

namespace qcm
{

auto image_response_count() -> std::atomic<i32>&;

class QcmImageResponse : public QQuickImageResponse {
public:
    ~QcmImageResponse() override { image_response_count()--; }
    auto errorString() const -> QString override { return m_error; }

    void set_error(QAnyStringView error) { m_error = error.toString(); }

    template<typename T>
    static auto make_rc() {
        return rc<T>(new T, rc_deleter);
    }

protected:
    QcmImageResponse() { image_response_count()++; }

private:
    void        done() { QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection); }
    static void rc_deleter(QcmImageResponse* p) { p->done(); }
    QString     m_error;
};

} // namespace qcm