#include "service_qml_ncm/model/fm_queue.h"
#include "ncm/client.h"
#include "service_qml_ncm/client.h"
#include "service_qml_ncm/model.h"

#include "qcm_interface/async.inl"
#include "qcm_interface/client.h"
#include "qcm_interface/action.h"
#include "ncm/api/v1_radio_get.h"

namespace ncm::qml
{
FmQueue::FmQueue(QObject* parent)
    : qcm::model::IdQueue(parent), m_query(new qcm::QAsyncResult(this)) {
    setOptions(Options(0));
    this->setName(u"Private Radio");
    connect(this, &qcm::model::IdQueue::requestNext, this, &FmQueue::onRequestNext);
}

FmQueue::~FmQueue() {}

void FmQueue::onRequestNext() {
    if (rowCount() >= 3) {
        if (currentIndex() > 0) {
            removeRow(0);
        }
        return;
    }
    auto self = helper::QWatcher { this };
    auto c    = get_ncm_client();
    if (! check(c)) return;

    m_query->spawn([self, c = *c] mutable -> qcm::task<void> {
        ncm::api::RadioGet api;
        auto               out = co_await c.perform(api);
        co_await asio::post(asio::bind_executor(qcm::qexecutor(), qcm::use_task));
        if (self) {
            auto q = self.get();
            self->m_query->check(out.transform([q](const auto& el) -> std::nullptr_t {
                auto view = std::views::transform(
                    el.data, [](const model::SongB& el) -> qcm::model::IdQueue::Item {
                        qcm::model::ItemId id;
                        QString            picUrl;
                        convert(id, el.id);
                        convert(picUrl, el.album.picUrl.value_or(""));
                        return { id, picUrl };
                    });
                std::vector<qcm::model::IdQueue::Item> items { view.begin(), view.end() };
                q->insert(q->rowCount(), items);
                return {};
            }));
            if (self->currentIndex() > 0) {
                self->removeRow(0);
            }
        }
        co_return;
    });
}
} // namespace ncm::qml