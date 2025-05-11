#include "Qcm/query/query.hpp"
#include "Qcm/status/provider_status.hpp"
#include "Qcm/app.hpp"

namespace qcm
{

QueryBase::QueryBase(QObject* parent): QAsyncResult(parent), m_delay(true) {
    set_forwardError(true);
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &QueryBase::reload);
}
QueryBase::~QueryBase() {}

auto QueryBase::create(std::string_view name) -> QueryBase* {
    auto q = QMetaType::fromName(name).create();
    _assert_rel_(q);
    return static_cast<QueryBase*>(q);
}

void QueryBase::request_reload() {
    if (delay()) {
        m_timer.setInterval(100);
        m_timer.start();
    } else {
        reload();
    }
}

auto QueryBase::delay() const -> bool { return m_delay; }
void QueryBase::setDelay(bool v) {
    if (ycore::cmp_exchange(m_delay, v)) {
        delayChanged();
    }
}
void QueryBase::connectSyncFinished() {
    connect(App::instance()->provider_status(),
            &ProviderStatusModel::syncingChanged,
            this,
            [this](bool syncing) {
                if (syncing) return;
                request_reload();
            });
}

QueryListBase::QueryListBase(QObject* parent): QueryBase(parent), m_offset(0), m_limit(200) {
    // connect_requet_reload(&QueryListBase::offsetChanged);
    // connect_requet_reload(&QueryListBase::limitChanged);
}
QueryListBase::~QueryListBase() {}
auto QueryListBase::offset() const -> qint32 { return m_offset; }
auto QueryListBase::limit() const -> qint32 { return m_limit; }

void QueryListBase::setOffset(qint32 v) {
    if (ycore::cmp_exchange(m_offset, v)) {
        offsetChanged();
    }
}
void QueryListBase::setLimit(qint32 v) {
    if (ycore::cmp_exchange(m_offset, v)) {
        limitChanged();
    }
}

void QueryListBase::fetchMore(qint32) { log::warn("fetchMore not impl"); }

void detail::try_connect_fetch_more(QObject* query, QObject* model) {
    if (model == nullptr) return;

    auto signal    = QMetaObject::normalizedSignature("reqFetchMore(qint32)");
    auto slot      = QMetaObject::normalizedSignature("fetchMore(qint32)");
    auto signalIdx = model->metaObject()->indexOfSignal(signal);
    auto slotIdx   = query->metaObject()->indexOfSlot(slot);
    if (signalIdx != -1 && slotIdx != -1) {
        QObject::connect(model, SIGNAL(reqFetchMore(qint32)), query, SLOT(fetchMore(qint32)));
    } else if (signalIdx != -1 || slotIdx != -1) {
        log::warn("reqFetchMore not connected");
    }
}

} // namespace qcm

#include <Qcm/query/moc_query.cpp>