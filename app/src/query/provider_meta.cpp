#include "Qcm/query/provider_meta.h"

#include "qcm_interface/async.inl"

#include "Qcm/app.h"

namespace qcm::query
{

ProviderMetasQuery::ProviderMetasQuery(QObject* parent)
    : QAsyncResultT<msg::GetProviderMetasRsp>(parent) {}
void ProviderMetasQuery::reload() {
    set_status(Status::Querying);
    auto backend = App::instance()->backend();
    auto self    = helper::QWatcher { this };
    spawn([self, backend] -> task<void> {
        msg::QcmMessage msg;
        msg.setType(msg::MessageTypeGadget::MessageType::GET_PROVIDER_METAS_REQ);
        msg.setGetProviderMetasReq(msg::GetProviderMetasReq {});
        auto rsp = co_await backend->send(std::move(msg));
        co_await qcm::qexecutor_switch();
        WARN_LOG("rsp id: {}", rsp.id_proto().t);
        co_return;
    });
}

} // namespace qcm::query

#include <Qcm/query/moc_provider_meta.cpp>
