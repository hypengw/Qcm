#include "qcm_interface/query.h"

namespace qcm::query
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

QueryListBase::QueryListBase(QObject* parent): QueryBase(parent), m_offset(0), m_limit(50) {
    connect_requet_reload(&QueryListBase::offsetChanged);
    connect_requet_reload(&QueryListBase::limitChanged);
}
QueryListBase::~QueryListBase() {}
auto QueryListBase::offset() const -> qint32 { return m_offset; }
auto QueryListBase::limit() const -> qint32 { return m_limit; }

Q_SLOT void QueryListBase::setOffset(qint32 v) {
    if (ycore::cmp_exchange(m_offset, v)) {
        offsetChanged();
    }
}
Q_SLOT void QueryListBase::setLimit(qint32 v) {
    if (ycore::cmp_exchange(m_offset, v)) {
        limitChanged();
    }
}

} // namespace qcm::query