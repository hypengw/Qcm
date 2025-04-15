#include "Qcm/query/provider_query.hpp"

#include "Qcm/util/async.inl"

#include "Qcm/app.hpp"

namespace qcm::query
{

AddProviderQuery::AddProviderQuery(QObject* parent): QAsyncResultT<msg::AddProviderRsp>(parent) {
    set_forwardError(true);
}
auto AddProviderQuery::failed() const -> const QString& { return m_failed; }
void AddProviderQuery::setFailed(QStringView v) {
    if (ycore::cmp_exchange(m_failed, v.toString())) {
        failedChanged();
    }
}

void AddProviderQuery::reload() {
    set_status(Status::Querying);
    auto backend = App::instance()->backend();
    auto self    = helper::QWatcher { this };
    spawn([self, backend, req = m_req] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        co_await qcm::qexecutor_switch();
        self->inspect_set(std::move(rsp), [&self](auto& rsp) {
            using R = qcm::msg::model::AuthResultGadget::AuthResult;
            switch (rsp.code()) {
            case R::Failed: {
                self->setFailed(rstd::into(std::format("Failed: {}", rsp.message())));
                break;
            }
            case R::NoSuchEmail: {
                self->setFailed(u"No such email");
                break;
            }
            case R::NoSuchPhone: {
                self->setFailed(u"No such phone");
                break;
            }
            case R::NoSuchUsername: {
                self->setFailed(u"No such username");
                break;
            }
            case R::WrongPassword: {
                self->setFailed(u"Wrong password");
                break;
            }
            default: {
                self->setFailed({});
            }
            }
            self->set_tdata(rsp);
        });
        co_return;
    });
}
auto AddProviderQuery::req() -> msg::AddProviderReq& { return m_req; }
void AddProviderQuery::setReq(msg::AddProviderReq& req) {
    if (ycore::cmp_exchange(m_req, req)) {
        reqChanged();
    }
}

ProviderMetasQuery::ProviderMetasQuery(QObject* parent)
    : QAsyncResultT<msg::GetProviderMetasRsp>(parent) {
    set_forwardError(true);
}
void ProviderMetasQuery::reload() {
    set_status(Status::Querying);
    auto backend = App::instance()->backend();
    auto self    = helper::QWatcher { this };
    spawn([self, backend] -> task<void> {
        auto rsp = co_await backend->send(msg::GetProviderMetasReq {});
        co_await qcm::qexecutor_switch();

        self->set(std::move(rsp));
        co_return;
    });
}

} // namespace qcm::query

#include <Qcm/query/moc_provider_query.cpp>
