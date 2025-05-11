#pragma once
#include <deque>
#include <QTimer>
#include "Qcm/util/async.hpp"
#include "Qcm/action.hpp"
#include "Qcm/notifier.hpp"

namespace qcm
{

class QueryBase : public QAsyncResult {
    Q_OBJECT

    Q_PROPERTY(bool delay READ delay WRITE setDelay NOTIFY delayChanged FINAL)
public:
    QueryBase(QObject* parent = nullptr);
    ~QueryBase();

    static auto create(std::string_view) -> QueryBase*;

    Q_SLOT void   request_reload();
    auto          delay() const -> bool;
    void          setDelay(bool v);
    Q_SIGNAL void delayChanged();

    void connectSyncFinished();

protected:
    template<typename T, typename R, typename... ARGS>
    void connect_requet_reload(R (T::*f)(ARGS...), T* obj) {
        connect(obj, f, this, &QueryBase::request_reload);
    }
    template<typename T, typename R, typename... ARGS>
    void connect_requet_reload(R (T::*f)(ARGS...)) {
        connect(static_cast<T*>(this), f, this, &QueryBase::request_reload);
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
    QueryListBase(QObject* parent = nullptr);
    ~QueryListBase();
    auto          offset() const -> qint32;
    auto          limit() const -> qint32;
    Q_SLOT void   setOffset(qint32 v);
    Q_SLOT void   setLimit(qint32 v);
    Q_SIGNAL void offsetChanged();
    Q_SIGNAL void limitChanged();

    Q_SLOT virtual void fetchMore(qint32);

private:
    qint32 m_offset;
    qint32 m_limit;
};

namespace detail
{
void try_connect_fetch_more(QObject* query, QObject* model);
}

template<typename T>
class Query : public QueryBase, public QAsyncResultExtra<T, Query<T>> {
public:
    Query(QObject* parent): QueryBase(parent), m_last(QDateTime::fromSecsSinceEpoch(0)) {
        if constexpr (std::is_base_of_v<QObject, T>) {
            detail::try_connect_fetch_more(this, this->tdata());
            if constexpr (std::constructible_from<T, QObject*>) {
                this->set_tdata(new T(this));
            } else {
                this->set_tdata(nullptr);
            }
        } else {
            this->set_tdata({});
        }
    }

    ~Query() override {}

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

    QDateTime m_last;
};

template<typename T>
class QueryList : public QueryListBase, public QAsyncResultExtra<T, Query<T>> {
public:
    QueryList(QObject* parent): QueryListBase(parent), m_last(QDateTime::fromSecsSinceEpoch(0)) {
        if constexpr (std::is_base_of_v<QObject, T>) {
            detail::try_connect_fetch_more(this, this->tdata());
            if constexpr (std::constructible_from<T, QObject*>) {
                this->set_tdata(new T(this));
            } else {
                this->set_tdata(nullptr);
            }
        } else {
            this->set_tdata({});
        }
    }

    ~QueryList() override {}

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

    QDateTime m_last;
};

} // namespace qcm