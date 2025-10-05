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
    void setError(QAnyStringView error);

    template<typename T, typename... Args>
    static auto make_rc(Args&&... args) {
        return rc<T>(new T(std::forward<Args>(args)...), rc_deleter);
    }

protected:
    QcmImageResponse();

private:
    void        done();
    static void rc_deleter(QcmImageResponse* p);

    QString m_error;
};

} // namespace qcm