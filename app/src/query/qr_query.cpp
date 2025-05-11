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
    req.setProviderMeta(m_type_name);
    auto self = helper::QWatcher { this };
    spawn([self, backend, req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->set(std::move(rsp));
        co_return;
    });
}
auto QrAuthUrlQuery::typeName() const -> QString { return m_type_name; }
void QrAuthUrlQuery::setTypeName(const QString& v) {
    if (ycore::cmp_exchange(m_type_name, v)) {
        typeNameChanged();
    }
}
} // namespace qcm

#include <Qcm/query/moc_qr_query.cpp>