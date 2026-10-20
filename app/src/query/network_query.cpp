module;
#include "Qcm/query/network_query.moc.h"
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
module qcm;
import :query.network;

namespace qcm
{
NetworkSettingsQuery::NetworkSettingsQuery(QObject* parent): Query(parent) {}
void NetworkSettingsQuery::reload() {
    setStatus(Status::Querying);
    auto self    = QWatcher { this };
    auto backend = App::instance()->backend();
    spawn([self, backend]() mutable -> task<void> {
        auto rsp = co_await backend->send(msg::GetNetworkSettingsReq {});
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self) co_return;
        if (! rsp) {
            self->setError(QString::fromStdString(rsp.unwrap_err_unchecked().message));
            self->setStatus(Status::Error);
            co_return;
        }
        auto proxies = QJsonDocument::fromJson(rsp->proxiesJson().toUtf8());
        if (! proxies.isArray()) {
            self->setError(tr("Invalid network settings response"));
            self->setStatus(Status::Error);
            co_return;
        }
        self->m_proxies  = proxies.array().toVariantList();
        self->m_revision = rsp->revision();
        self->settingsChanged();
        self->setError({});
        self->setStatus(Status::Finished);
    });
}
UpdateNetworkSettingsQuery::UpdateNetworkSettingsQuery(QObject* parent): Query(parent) {}
void UpdateNetworkSettingsQuery::save(const QVariantList& proxies, quint64 revision) {
    if (querying()) return;
    m_req.setProxiesJson(
        QString::fromUtf8(QJsonDocument::fromVariant(proxies).toJson(QJsonDocument::Compact)));
    m_req.setRevision(revision);
    reload();
}
void UpdateNetworkSettingsQuery::reload() {
    if (querying()) return;
    setStatus(Status::Querying);
    auto self    = QWatcher { this };
    auto backend = App::instance()->backend();
    spawn([self, backend, req = m_req]() mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self) co_return;
        self->set(std::move(rsp));
    });
}
} // namespace qcm
#include "Qcm/query/network_query.moc.cpp"
