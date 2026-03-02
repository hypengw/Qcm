module;

export module qcm:image_provider.response;
export import qcm.core;
export import qt;

using rstd::sync::atomic::Atomic;

namespace qcm
{

auto image_response_count() -> Atomic<i32>&;

class QcmImageResponse : public QQuickImageResponse {
public:
    ~QcmImageResponse();
    auto errorString() const -> QString;
    void setError(QAnyStringView error);

    template<typename T, typename... Args>
    static auto make_rc(Args&&... args) {
        return rc<T>(new T(rstd::forward<Args>(args)...), rc_deleter);
    }

protected:
    QcmImageResponse();

private:
    void        done();
    static void rc_deleter(QcmImageResponse* p);

    QString m_error;
};

} // namespace qcm