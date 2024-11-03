#pragma once
#include <QTimer>
#include "qcm_interface/sync_api.h"
#include "qcm_interface/async.h"

namespace qcm::query
{

namespace detail
{

class QueryBase : public QAsyncResult {
    Q_OBJECT
public:
    QueryBase(QObject* parent = nullptr): QAsyncResult(parent) {
        set_forwardError(true);
        m_timer.setSingleShot(true);
        connect(&m_timer, &QTimer::timeout, this, &QueryBase::reload);
    }
    ~QueryBase() {}

    Q_SLOT void request_reload() {
        m_timer.setInterval(300);
        m_timer.start();
    }

protected:
    template<typename T, typename R, typename... ARGS>
    void connect_requet_reload(R (T::*f)(ARGS...), T* obj = nullptr) {
        connect(obj == nullptr ? static_cast<T*>(this) : obj, f, this, &QueryBase::request_reload);
    }

private:
    QTimer m_timer;
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
public:
    Query(QObject* parent = nullptr): Base(parent), m_data(new T(this)) {}
    ~Query() {}

    auto data() const -> QVariant override { return QVariant::fromValue(m_data); }
    auto tdata() const -> T* { return m_data; }

private:
    T* m_data;
};

template<typename T>
using QueryList = Query<T, detail::QueryListBase>;

} // namespace qcm::query