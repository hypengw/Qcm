module;
#undef assert
#include <rstd/macro.hpp>
#include "core/log.h"
#include "Qcm/query/query.moc.h"
module qcm;
import qcm.log;
import :query.query;
import :global;
import :action;
import :notifier;
import :util.async;

namespace qcm
{

class QAsyncResult::Private {
public:
    Private(QAsyncResult* p)
        : m_p(p),
          m_forward_error(true),
          m_data(QVariant::fromValue(nullptr)),
          m_use_queue(false),
          m_queue_exec_mark(false),
          m_querying(false, p),
          m_status(Status::Uninitialized, p),
          m_error(p) {}
    QAsyncResult*            m_p;
    bool                     m_forward_error;
    QVariant                 m_data;
    cppstd::function<void()> m_cb;

    WatchDog                                       m_wdog;
    cppstd::map<QString, QObject*, cppstd::less<>> m_hold;

    bool m_use_queue;
    bool m_queue_exec_mark;
    cppstd::deque<cppstd::tuple<cppstd::function<task<void>()>, cppstd::source_location>> m_queue;

    ObjectBindableProperty<QAsyncResult, bool, &QAsyncResult ::queryingChanged> m_querying;
    ObjectBindableProperty<QAsyncResult, Status, &QAsyncResult ::statusChanged> m_status;
    ObjectBindableProperty<QAsyncResult, QString, &QAsyncResult::errorChanged>  m_error;

    void try_run() {
        if (m_queue.empty() || m_queue_exec_mark || ! m_use_queue) return;

        auto [f, loc] = m_queue.front();
        m_queue.pop_front();

        auto                   ex = asio::make_strand(m_p->pool_executor());
        QWatcher<QAsyncResult> self { m_p };
        auto                   main_ex { m_p->get_executor() };
        auto                   alloc = asio::recycling_allocator<void>();

        m_p->setStatus(Status::Querying);
        m_queue_exec_mark = true;
        asio::co_spawn(ex,
                       m_p->watch_dog().watch(
                           ex,
                           [f = std::move(f)] -> task<void> {
                               co_await f();
                           },
                           asio::chrono::minutes(3),
                           alloc),
                       asio::bind_allocator(alloc, [self, main_ex, loc](std::exception_ptr p) {
                           if (p) {
                               try {
                                   std::rethrow_exception(p);
                               } catch (const std::exception& e) {
                                   std::string e_str = e.what();
                                   asio::post(main_ex, [self, e_str]() {
                                       if (self) {
                                           self->setError(QString::fromStdString(e_str));
                                           self->setStatus(Status::Error);
                                       }
                                   });
                                   log::log(LogLevel::ERROR, loc, "{}", e_str);
                               }
                           }

                           asio::post(main_ex, [self] {
                               if (self) {
                                   self->d_func()->handle_queue();
                               }
                           });
                       }));
    }

    void handle_queue() {
        m_queue_exec_mark = false;
        try_run();
    }
};

QAsyncResult::QAsyncResult(QObject* parent): QObject(parent), d_ptr(make_up<Private>(this)) {
    C_D(QAsyncResult);
    connect(this, &QAsyncResult::statusChanged, this, [this](Status s) {
        if (s == Status::Finished) {
            finished();
        } else if (s == Status::Error) {
            errorOccurred(error());
        }
        if (forwardError() && s == Status::Error) {
            emit Global::instance() -> errorOccurred(error());
        }
    });

    d->m_querying.setBinding([d] {
        return d->m_status.value() == Status::Querying;
    });
}

QAsyncResult::~QAsyncResult() {}

void QAsyncResult::hold(QStringView name, QObject* o) {
    C_D(QAsyncResult);
    if (o != nullptr) {
        o->setParent(this);
        if (auto it = d->m_hold.find(name); it != d->m_hold.end()) {
            it->second->deleteLater();
            it->second = o;
        } else {
            d->m_hold.insert({ name.toString(), o });
        }
    }
}

auto QAsyncResult::qexecutor() const -> QtExecutor& { return qcm::qexecutor(); }

auto QAsyncResult::pool_executor() const -> asio::thread_pool::executor_type {
    return Global::instance()->pool_executor();
}

auto QAsyncResult::status() const -> Status {
    C_D(const QAsyncResult);
    return d->m_status.value();
}
auto QAsyncResult::bindableStatus() -> QBindable<Status> {
    C_D(QAsyncResult);
    return &(d->m_status);
}
auto QAsyncResult::querying() const -> bool {
    C_D(const QAsyncResult);
    return d->m_querying.value();
}
auto QAsyncResult::bindableQuerying() -> QBindable<bool> {
    C_D(QAsyncResult);
    return &(d->m_querying);
}

void QAsyncResult::setStatus(Status v) {
    C_D(QAsyncResult);
    d->m_status = v;
}
void QAsyncResult::reload() {
    C_D(const QAsyncResult);
    if (d->m_cb) {
        d->m_cb();
    }
}
void QAsyncResult::set_reload_callback(const std::function<void()>& f) {
    C_D(QAsyncResult);
    d->m_cb = f;
}

auto QAsyncResult::error() const -> const QString& {
    C_D(const QAsyncResult);
    return d->m_error.value();
}
auto QAsyncResult::bindableError() -> QBindable<QString> {
    C_D(QAsyncResult);
    return &(d->m_error);
}
void QAsyncResult::setError(const QString& v) {
    C_D(QAsyncResult);
    d->m_error = v;
}

bool QAsyncResult::forwardError() const {
    C_D(const QAsyncResult);
    return d->m_forward_error;
}
void QAsyncResult::setForwardError(bool v) {
    C_D(QAsyncResult);
    if (ycore::cmp_set(d->m_forward_error, v)) {
        emit forwardErrorChanged();
    }
}
void QAsyncResult::cancel() {
    C_D(QAsyncResult);
    d->m_wdog.cancel();
}
auto QAsyncResult::get_executor() -> QtExecutor& {
    C_D(QAsyncResult);
    return qcm::qexecutor();
}
auto QAsyncResult::use_queue() const -> bool {
    C_D(const QAsyncResult);
    return d->m_use_queue;
}
void QAsyncResult::set_use_queue(bool v) {
    C_D(QAsyncResult);
    d->m_use_queue = v;
}

auto QAsyncResult::watch_dog() -> WatchDog& {
    C_D(QAsyncResult);
    return d->m_wdog;
}

auto QAsyncResult::data() const -> const QVariant& {
    C_D(const QAsyncResult);
    return d->m_data;
}

auto QAsyncResult::data() -> QVariant& {
    C_D(QAsyncResult);
    return d->m_data;
}

void QAsyncResult::set_data(const QVariant& v) {
    C_D(QAsyncResult);
    if (ycore::cmp_set(d->m_data, v)) {
        dataChanged();
    }
    if (auto obj = d->m_data.value<QObject*>(); obj != nullptr) {
        if (obj->parent() != this) {
            obj->setParent(this);
        }
    }
}
void QAsyncResult::push(std::function<task<void>()> in, const cppstd::source_location& loc) {
    C_D(QAsyncResult);
    d->m_queue.emplace_back(in, loc);

    d->try_run();
}

usize QAsyncResult::size() const {
    C_D(const QAsyncResult);
    return d->m_queue.size();
}


Query::Query(QObject* parent): QAsyncResult(parent), m_delay(true) {
    setForwardError(true);
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &Query::reload);
}
Query::~Query() {}

auto Query::create(std::string_view name) -> Query* {
    auto q = QMetaType::fromName(name).create();
    assert(q);
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

void QueryList::fetchMore(qint32) { LOG_WARN("fetchMore not impl"); }

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

#include "Qcm/query/query.moc.cpp"
