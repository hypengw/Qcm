#pragma once

#include <QtQuick/QQuickImageResponse>
#include "core/core.h"

namespace qcm
{

auto image_response_count() -> std::atomic<i32>&;

class QcmImageResponse : public QQuickImageResponse {
public:
    ~QcmImageResponse();
    auto errorString() const -> QString;
    void set_error(QAnyStringView error);

    template<typename T>
    static auto make_rc() {
        return rc<T>(new T, rc_deleter);
    }

protected:
    QcmImageResponse();

private:
    void        done();
    static void rc_deleter(QcmImageResponse* p);

    QString m_error;
};

} // namespace qcm