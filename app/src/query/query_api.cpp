#include "Qcm/query/query_api.hpp"
#include "Qcm/global.hpp"

#include "core/qobject_bindable_property_p.h"

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
    QAsyncResult*         m_p;
    bool                  m_forward_error;
    QVariant              m_data;
    std::function<void()> m_cb;

    helper::WatchDog                         m_wdog;
    std::map<QString, QObject*, std::less<>> m_hold;

    bool                                                                      m_use_queue;
    bool                                                                      m_queue_exec_mark;
    std::deque<std::tuple<std::function<task<void>()>, std::source_location>> m_queue;

    ObjectBindableProperty<QAsyncResult, bool, &QAsyncResult ::queryingChanged> m_querying;
    ObjectBindableProperty<QAsyncResult, Status, &QAsyncResult ::statusChanged> m_status;
    ObjectBindableProperty<QAsyncResult, QString, &QAsyncResult::errorChanged>  m_error;

    void try_run() {
        if (m_queue.empty() || m_queue_exec_mark || ! m_use_queue) return;

        auto [f, loc] = m_queue.front();
        m_queue.pop_front();

        auto                           ex = asio::make_strand(m_p->pool_executor());
        helper::QWatcher<QAsyncResult> self { m_p };
        auto                           main_ex { m_p->get_executor() };
        auto                           alloc = asio::recycling_allocator<void>();

        m_p->set_status(Status::Querying);
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
                                           self->set_error(QString::fromStdString(e_str));
                                           self->set_status(Status::Error);
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

void QAsyncResult::set_status(Status v) {
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
void QAsyncResult::set_error(const QString& v) {
    C_D(QAsyncResult);
    d->m_error = v;
}

bool QAsyncResult::forwardError() const {
    C_D(const QAsyncResult);
    return d->m_forward_error;
}
void QAsyncResult::set_forwardError(bool v) {
    C_D(QAsyncResult);
    if (ycore::cmp_exchange(d->m_forward_error, v)) {
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

auto QAsyncResult::watch_dog() -> helper::WatchDog& {
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
    if (ycore::cmp_exchange(d->m_data, v)) {
        dataChanged();
    }
    if (auto obj = d->m_data.value<QObject*>(); obj != nullptr) {
        if (obj->parent() != this) {
            obj->setParent(this);
        }
    }
}
void QAsyncResult::push(std::function<task<void>()> in, const std::source_location& loc) {
    C_D(QAsyncResult);
    d->m_queue.emplace_back(in, loc);

    d->try_run();
}

usize QAsyncResult::size() const {
    C_D(const QAsyncResult);
    return d->m_queue.size();
}

class ApiQueryBase::Private {
public:
    Private(): m_auto_reload(true), m_dirty(true) {}

    ~Private() {}

    bool m_auto_reload;
    bool m_qml_parsing;
    bool m_dirty;

};

ApiQueryBase::ApiQueryBase(QObject* parent): QAsyncResult(parent), d_ptr(make_up<Private>()) {
    connect(this,
            &ApiQueryBase::autoReloadChanged,
            this,
            &ApiQueryBase::reload_if_needed,
            Qt::DirectConnection);
}

ApiQueryBase::~ApiQueryBase() {}

bool ApiQueryBase::autoReload() const {
    C_D(const ApiQueryBase);
    return d->m_auto_reload;
}

void ApiQueryBase::set_autoReload(bool v) {
    C_D(ApiQueryBase);
    if (std::exchange(d->m_auto_reload, v) != v) {
        emit autoReloadChanged();
    }
}

void ApiQueryBase::classBegin() {
    C_D(ApiQueryBase);
    d->m_qml_parsing = true;
}
void ApiQueryBase::componentComplete() {
    C_D(ApiQueryBase);
    d->m_qml_parsing = false;
    reload_if_needed();
}

bool ApiQueryBase::is_qml_parsing() const {
    C_D(const ApiQueryBase);
    return d->m_qml_parsing;
}

bool ApiQueryBase::dirty() const {
    C_D(const ApiQueryBase);
    return d->m_dirty;
}
void ApiQueryBase::mark_dirty(bool v) {
    C_D(ApiQueryBase);
    d->m_dirty = v;
}

void ApiQueryBase::reload_if_needed() {
    if (! is_qml_parsing() && autoReload() && dirty()) {
        reload();
        mark_dirty(false);
    }
}

void ApiQueryBase::fetch_more(qint32) {}

void ApiQueryBase::query() { reload(); }

} // namespace qcm

#include <Qcm/util/moc_async.cpp>
#include <Qcm/query/moc_query_api.cpp>