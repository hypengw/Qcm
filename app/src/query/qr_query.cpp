#include "Qcm/query/qr_query.hpp"
#include "Qcm/app.hpp"
#include "Qcm/util/async.inl"

namespace qcm
{
QrAuthUrlQuery::QrAuthUrlQuery(QObject* parent): Query(parent) {}
void QrAuthUrlQuery::reload() {
    set_status(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::QrAuthUrlReq {};
    req.setTmpProvider(m_tmp_provider);
    auto self = helper::QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        // ignore error
        if (rsp) {
            self->set(std::move(rsp));
        }
        co_return;
    });
}

auto QrAuthUrlQuery::tmpProvider() const -> QString { return m_tmp_provider; }
void QrAuthUrlQuery::setTmpProvider(const QString& v) {
    if (ycore::cmp_exchange(m_tmp_provider, v)) {
        tmpProviderChanged();
    }
}
} // namespace qcm

#include <Qcm/query/moc_qr_query.cpp>