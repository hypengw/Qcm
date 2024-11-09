#pragma once
#include <deque>
#include <QTimer>
#include "qcm_interface/sync_api.h"
#include "qcm_interface/async.h"
#include "qcm_interface/action.h"
#include "qcm_interface/notifier.h"

namespace qcm::query
{

namespace detail
{

class QueryBase : public QAsyncResult {
    Q_OBJECT

    Q_PROPERTY(bool delay READ delay WRITE setDelay NOTIFY delayChanged FINAL)
public:
    QueryBase(QObject* parent = nullptr): QAsyncResult(parent), m_delay(true) {
        set_forwardError(true);
        m_timer.setSingleShot(true);
        connect(&m_timer, &QTimer::timeout, this, &QueryBase::reload);
    }
    ~QueryBase() {}

    Q_SLOT void request_reload() {
        if (delay()) {
            m_timer.setInterval(100);
            m_timer.start();
        } else {
            reload();
        }
    }

    auto delay() const -> bool { return m_delay; }
    void setDelay(bool v) {
        if (ycore::cmp_exchange(m_delay, v)) {
            delayChanged();
        }
    }
    Q_SIGNAL void delayChanged();

protected:
    template<typename T, typename R, typename... ARGS>
    void connect_requet_reload(R (T::*f)(ARGS...), T* obj = nullptr) {
        connect(obj == nullptr ? static_cast<T*>(this) : obj, f, this, &QueryBase::request_reload);
    }

private:
    QTimer m_timer;
    bool   m_delay;

    std::deque<std::function<task<void>()>> m_queue;
};

class QueryListBase : public QueryBase {
    Q_OBJECT
    Q_PROPERTY(qint32 offset READ offset WRITE setOffset NOTIFY offsetChanged)
    Q_PROPERTY(qint32 limit READ limit WRITE setLimit NOTIFY limitChanged)
public:
    QueryListBase(QObject* parent = nullptr): QueryBase(parent), m_offset(0), m_limit(50) {
        connect_requet_reload(&QueryListBase::offsetChanged);
        connect_requet_reload(&QueryListBase::limitChanged);
    }
    ~QueryListBase() {}
    auto offset() const -> qint32 { return m_offset; }
    auto limit() const -> qint32 { return m_limit; }

    Q_SLOT void setOffset(qint32 v) {
        if (ycore::cmp_exchange(m_offset, v)) {
            offsetChanged();
        }
    }
    Q_SLOT void setLimit(qint32 v) {
        if (ycore::cmp_exchange(m_offset, v)) {
            limitChanged();
        }
    }

    Q_SIGNAL void offsetChanged();
    Q_SIGNAL void limitChanged();

private:
    qint32 m_offset;
    qint32 m_limit;
};
} // namespace detail

template<typename T, typename Base = detail::QueryBase>
class Query : public Base {
    static auto createData(QObject* q) {
        if constexpr (std::is_base_of_v<QObject, T>) {
            return new T(q);
        } else {
            return T();
        }
    }

public:
    using value_type        = decltype(Query::createData(nullptr));
    using const_value_type  = std::add_const_t<value_type>;
    using borrow_type       = std::conditional_t<std::is_pointer_v<value_type>, value_type,
                                                 std::add_lvalue_reference_t<value_type>>;
    using const_borrow_type = std::conditional_t<std::is_pointer_v<value_type>, value_type,
                                                 std::add_lvalue_reference_t<const_value_type>>;

    Query(QObject* parent = nullptr)
        : Base(parent), m_data(createData(this)), m_last(QDateTime::fromSecsSinceEpoch(0)) {}
    ~Query() {}

    auto data() const -> QVariant override { return QVariant::fromValue(m_data); }
    auto tdata() const -> const_borrow_type { return m_data; }
    auto tdata() -> borrow_type { return m_data; }
    void set_tdata(const_borrow_type v) {
        m_data = v;
        this->dataChanged();
    }

    auto last() const -> const QDateTime& { return m_last; }
    void setLast(const QDateTime& t, const QDateTime& last = QDateTime::currentDateTime()) {
        m_last = std::min<QDateTime>(t, last);
    }
    void resetLast() { m_last = QDateTime::fromSecsSinceEpoch(0); }
    auto record() {
        auto old = m_last;
        m_last   = QDateTime::currentDateTimeUtc();
        return old;
    }

    value_type m_data;
    QDateTime  m_last;
};

template<typename T>
using QueryList = Query<T, detail::QueryListBase>;

} // namespace qcm::query