#include "Qcm/query/query.hpp"
#include "Qcm/status/provider_status.hpp"
#include "Qcm/app.hpp"

namespace qcm
{

Query::Query(QObject* parent): QAsyncResult(parent), m_delay(true) {
    setForwardError(true);
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &Query::reload);
}
Query::~Query() {}

auto Query::create(std::string_view name) -> Query* {
    auto q = QMetaType::fromName(name).create();
    _assert_rel_(q);
    return static_cast<Query*>(q);
}

void Query::delayReload() {
    if (delay()) {
        m_timer.setInterval(100);
        m_timer.start();
    } else {
        reload();
    }
}

auto Query::delay() const -> bool { return m_delay; }
void Query::setDelay(bool v) {
    if (ycore::cmp_set(m_delay, v)) {
        delayChanged();
    }
}
void Query::connectSyncFinished() {
    connect(App::instance()->provider_status(),
            &ProviderStatusModel::syncingChanged,
            this,
            [this](bool syncing) {
                if (syncing) return;
                delayReload();
            });
}

QueryList::QueryList(QObject* parent)
    : Query(parent), m_offset(0), m_limit(200), m_sort(0), m_asc(true), m_no_more(false) {}

QueryList::~QueryList() {}
auto QueryList::offset() const noexcept -> qint32 { return m_offset; }
auto QueryList::limit() const noexcept -> qint32 { return m_limit; }
auto QueryList::endOffset() const noexcept -> qint32 { return (m_offset + 1) * m_limit; }

void QueryList::setOffset(qint32 v) {
    if (ycore::cmp_set(m_offset, v)) {
        offsetChanged();
    }
}
void QueryList::setLimit(qint32 v) {
    if (ycore::cmp_set(m_limit, v)) {
        limitChanged();
    }
}
auto QueryList::sort() const noexcept -> qint32 { return m_sort; }
void QueryList::setSort(qint32 v) {
    if (ycore::cmp_set(m_sort, v)) {
        sortChanged();
    }
}
auto QueryList::asc() const noexcept -> bool { return m_asc; }
void QueryList::setAsc(bool v) {
    if (ycore::cmp_set(m_asc, v)) {
        ascChanged();
    }
}

auto QueryList::noMore() const noexcept -> bool { return m_no_more; }
void QueryList::setNoMore(bool v) {
    if (v != m_no_more) {
        m_no_more = v;
        noMoreChanged();
    }
}

void QueryList::fetchMore(qint32) { log::warn("fetchMore not impl"); }

void detail::try_connect_fetch_more(QObject* query, QObject* model) {
    if (model == nullptr) return;

    auto signal    = QMetaObject::normalizedSignature("reqFetchMore(qint32)");
    auto slot      = QMetaObject::normalizedSignature("fetchMore(qint32)");
    auto signalIdx = model->metaObject()->indexOfSignal(signal);
    auto slotIdx   = query->metaObject()->indexOfSlot(slot);
    if (signalIdx != -1 && slotIdx != -1) {
        QObject::connect(model, SIGNAL(reqFetchMore(qint32)), query, SLOT(fetchMore(qint32)));
    }
}

} // namespace qcm

#include <Qcm/query/moc_query.cpp>