module;
#include "Qcm/query/qr_query.moc.h"
module qcm;
import :query.qr;

namespace qcm
{

QrAuthUrlQuery::QrAuthUrlQuery(QObject* parent): Query(parent) {}
void QrAuthUrlQuery::reload() {
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto req     = msg::QrAuthUrlReq {};
    req.setTmpProvider(m_tmp_provider);
    auto self = QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        if (rsp) {
            self->set(std::move(rsp));
        }
        co_return;
    });
}

auto QrAuthUrlQuery::tmpProvider() const -> QString { return m_tmp_provider; }
void QrAuthUrlQuery::setTmpProvider(const QString& v) {
    if (ycore::cmp_set(m_tmp_provider, v)) {
        tmpProviderChanged();
    }
}

} // namespace qcm

#include "Qcm/query/qr_query.moc.cpp"
