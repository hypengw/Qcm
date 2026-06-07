module;
#include <mutex>
#include <optional>

export module qcm:image_provider.response;
import qcm.core;
import qt;

using rstd::sync::atomic::Atomic;

namespace qcm
{

auto image_response_count() -> Atomic<i32>&;

class QcmImageResponse : public QQuickImageResponse {
public:
    ~QcmImageResponse();
    auto errorString() const -> QString override;
    void cancel() override;
    void setError(QAnyStringView error);
    void set_task(rstd::async::JoinHandle<void>);

    template<typename T, typename... Args>
    static auto make_rc(Args&&... args) {
        return rc<T>(new T(rstd::forward<Args>(args)...), rc_deleter);
    }

protected:
    QcmImageResponse();

private:
    void        done();
    static void rc_deleter(QcmImageResponse* p);

    QString                                      m_error;
    std::mutex                                   m_task_mutex;
    std::optional<rstd::async::JoinHandle<void>> m_task;
    bool                                         m_canceled { false };
};

} // namespace qcm
