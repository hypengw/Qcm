#pragma once
#include <deque>
#include <QTimer>
#include "qcm_interface/sync_api.h"
#include "qcm_interface/async.h"
#include "qcm_interface/action.h"
#include "qcm_interface/notifier.h"

namespace qcm::query
{

class QCM_INTERFACE_API QueryBase : public QAsyncResult {
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

class QCM_INTERFACE_API QueryListBase : public QueryBase {
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

template<typename T, typename Base = QueryBase>
class Query : public QAsyncResultT<T, Base> {
public:
    Query(ycore::monostate m, QObject* parent, QAsyncResultT<T, Base>::const_reference_value_type t)
        : QAsyncResultT<T, Base>(m, parent, t), m_last(QDateTime::fromSecsSinceEpoch(0)) {
        if constexpr (std::is_base_of_v<QObject, T>) {
            detail::try_connect_fetch_more(this, this->tdata());
        }
    }

    template<typename... Arg>
        requires std::is_constructible_v<T, Arg...>
    Query(QObject* parent, Arg&&... arg)
        : QAsyncResultT<T, Base>(parent, std::forward<Arg>(arg)...),
          m_last(QDateTime::fromSecsSinceEpoch(0)) {
        if constexpr (std::is_base_of_v<QObject, T>) {
            detail::try_connect_fetch_more(this, this->tdata());
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
using QueryList = Query<T, QueryListBase>;

} // namespace qcm::query