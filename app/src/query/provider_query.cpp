#include "Qcm/query/provider_query.hpp"

#include "Qcm/util/async.inl"

#include "Qcm/app.hpp"

namespace qcm
{

namespace
{
template<typename T>
auto format_auth_res(const T& rsp) -> std::string {
    std::string out;
    using R = qcm::msg::model::AuthResultGadget::AuthResult;
    switch (rsp.code()) {
    case R::Failed: {
        out = std::format("Failed: {}", rsp.message());
        break;
    }
    case R::NoSuchEmail: {
        out = "No such email";
        break;
    }
    case R::NoSuchPhone: {
        out = "No such phone";
        break;
    }
    case R::NoSuchUsername: {
        out = "No such username";
        break;
    }
    case R::WrongPassword: {
        out = "Wrong password";
        break;
    }
    default: {
    }
    }
    return out;
}
} // namespace

auto AuthProviderQuery::failed() const -> const QString& { return m_failed; }
void AuthProviderQuery::setFailed(QStringView v) {
    if (ycore::cmp_set(m_failed, v.toString())) {
        failedChanged();
    }
}
AuthProviderQuery::AuthProviderQuery(QObject* parent): Query(parent) { setForwardError(true); }
void AuthProviderQuery::reload() {
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto self    = helper::QWatcher { this };
    m_req.setTmpProvider(m_tmp_provider);
    spawn([self, backend, req = m_req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->inspect_set(std::move(rsp), [&self](auto& rsp) {
            QString error = rstd::into(format_auth_res(rsp));
            self->setFailed(error);
            self->set_tdata(rsp);
        });
        co_return;
    });
}
auto AuthProviderQuery::req() -> msg::AuthProviderReq& { return m_req; }
void AuthProviderQuery::setReq(msg::AuthProviderReq& req) {
    if (ycore::cmp_set(m_req, req)) {
        reqChanged();
    }
}
auto AuthProviderQuery::tmpProvider() const -> QString { return m_tmp_provider; }
void AuthProviderQuery::setTmpProvider(const QString& v) {
    if (ycore::cmp_set(m_tmp_provider, v)) {
        tmpProviderChanged();
    }
}

ProviderMetasQuery::ProviderMetasQuery(QObject* parent): Query(parent) { setForwardError(true); }
void ProviderMetasQuery::reload() {
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto self    = helper::QWatcher { this };
    spawn([self, backend] -> task<void> {
        auto rsp = co_await backend->send(msg::GetProviderMetasReq {});
        co_await qcm::qexecutor_switch();

        self->set(std::move(rsp));
        co_return;
    });
}

AddProviderQuery::AddProviderQuery(QObject* parent): Query(parent) { setForwardError(true); }
auto AddProviderQuery::req() -> msg::AddProviderReq& { return m_req; }
void AddProviderQuery::setReq(msg::AddProviderReq& req) {
    if (ycore::cmp_set(m_req, req)) {
        reqChanged();
    }
}

void AddProviderQuery::reload() {
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto self    = helper::QWatcher { this };
    spawn([self, backend, req = m_req]() mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->set(std::move(rsp));
        co_return;
    });
}

UpdateProviderQuery::UpdateProviderQuery(QObject* parent): Query(parent) { setForwardError(true); }

auto UpdateProviderQuery::req() -> msg::UpdateProviderReq& { return m_req; }
void UpdateProviderQuery::setReq(const msg::UpdateProviderReq& req) {
    if (ycore::cmp_set(m_req, req)) {
        reqChanged();
    }
}
auto UpdateProviderQuery::providerId() const -> model::ItemId { return m_provider_id; }
void UpdateProviderQuery::setProviderId(const model::ItemId& id) {
    if (ycore::cmp_set(m_provider_id, id)) {
        providerIdChanged();
    }
}

void UpdateProviderQuery::reload() {
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto self    = helper::QWatcher { this };
    m_req.setProviderId(m_provider_id.id());
    spawn([self, backend, req = m_req]() mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        if (rsp) {
            auto error = format_auth_res(*rsp);
            if (! error.empty()) {
                self->setError(rstd::into(error));
                self->setStatus(Status::Error);
            } else {
                self->set(std::move(rsp));
            }
        } else {
            self->set(std::move(rsp));
        }
        co_return;
    });
}

DeleteProviderQuery::DeleteProviderQuery(QObject* parent): Query(parent) { setForwardError(true); }
auto DeleteProviderQuery::providerId() const -> model::ItemId { return m_provider_id; }
void DeleteProviderQuery::setProviderId(const model::ItemId& id) {
    if (ycore::cmp_set(m_provider_id, id)) {
        providerIdChanged();
    }
}
void DeleteProviderQuery::reload() {
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto self    = helper::QWatcher { this };
    m_req.setProviderId(m_provider_id.id());
    spawn([self, backend, req = m_req]() mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->set(std::move(rsp));
        co_return;
    });
}

ReplaceProviderQuery::ReplaceProviderQuery(QObject* parent): Query(parent) {
    setForwardError(true);
}

auto ReplaceProviderQuery::providerId() const -> model::ItemId { return m_provider_id; }
void ReplaceProviderQuery::setProviderId(const model::ItemId& id) {
    if (ycore::cmp_set(m_provider_id, id)) {
        providerIdChanged();
    }
}
auto ReplaceProviderQuery::tmpProvider() const -> QString { return m_tmp_provider; }
void ReplaceProviderQuery::setTmpProvider(const QString& v) {
    if (v != m_tmp_provider) {
        m_tmp_provider = v;
        tmpProviderChanged();
    }
}

void ReplaceProviderQuery::reload() {
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto self    = helper::QWatcher { this };
    auto req     = msg::ReplaceProviderReq {};
    req.setProviderId(m_provider_id.id());
    req.setTmpProvider(m_tmp_provider);
    spawn([self, backend, req]() mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->set(std::move(rsp));
        co_return;
    });
}

CreateTmpProviderQuery::CreateTmpProviderQuery(QObject* parent): Query(parent) {
    setForwardError(true);
}
CreateTmpProviderQuery::~CreateTmpProviderQuery() {
    auto    backend = App::instance()->backend();
    QString key     = this->tdata().key();
    if (key.isEmpty()) return;
    spawn([backend, key]() mutable -> task<void> {
        auto req = msg::DeleteTmpProviderReq {};
        req.setKey(key);
        co_await backend->send(std::move(req));
        co_return;
    });
}
auto CreateTmpProviderQuery::typeName() const -> QString { return m_type_name; }
void CreateTmpProviderQuery::setTypeName(const QString& v) {
    if (ycore::cmp_set(m_type_name, v)) {
        typeNameChanged();
    }
}
void CreateTmpProviderQuery::reload() {
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto self    = helper::QWatcher { this };
    spawn([self, backend, t = m_type_name]() mutable -> task<void> {
        auto req = msg::CreateTmpProviderReq {};
        req.setTypeName(t);
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->set(std::move(rsp));
        co_return;
    });
}

DeleteTmpProviderQuery::DeleteTmpProviderQuery(QObject* parent): Query(parent) {
    setForwardError(true);
}
auto DeleteTmpProviderQuery::req() -> msg::DeleteTmpProviderReq& { return m_req; }
void DeleteTmpProviderQuery::setReq(msg::DeleteTmpProviderReq& req) {
    if (ycore::cmp_set(m_req, req)) {
        reqChanged();
    }
}
void DeleteTmpProviderQuery::reload() {
    setStatus(Status::Querying);
    auto backend = App::instance()->backend();
    auto self    = helper::QWatcher { this };
    spawn([self, backend, req = m_req]() mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->set(std::move(rsp));
        co_return;
    });
}

} // namespace qcm

#include <Qcm/query/moc_provider_query.cpp>
