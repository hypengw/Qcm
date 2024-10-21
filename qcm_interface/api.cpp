#include "qcm_interface/api.h"
#include "qcm_interface/global.h"

namespace qcm
{

class QAsyncResult::Private {
public:
    Private(): m_status(Status::Uninitialized), m_forward_error(true), m_data(nullptr) {}

    Status                m_status;
    bool                  m_forward_error;
    QObject*              m_data;
    std::function<void()> m_cb;

    helper::WatchDog                         m_wdog;
    QString                                  m_error;
    std::map<QString, QObject*, std::less<>> m_hold;
};

QAsyncResult::QAsyncResult(QObject* parent): QObject(parent), d_ptr(make_up<Private>()) {}
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

auto QAsyncResult::qexecutor() const -> QtExecutor& { return Global::instance()->qexecutor(); }

auto QAsyncResult::pool_executor() const -> asio::thread_pool::executor_type {
    return Global::instance()->pool_executor();
}

auto QAsyncResult::status() const -> Status {
    C_D(const QAsyncResult);
    return d->m_status;
}

void QAsyncResult::set_status(Status v) {
    C_D(QAsyncResult);
    if (ycore::cmp_exchange(d->m_status, v)) {
        Q_EMIT statusChanged(d->m_status);
        if (d->m_status == Status::Finished) {
            Q_EMIT finished();
        } else if (d->m_status == Status::Error) {
            Q_EMIT errorOccurred(d->m_error);
        }
        if (forwardError() && d->m_status == Status::Error) {
            emit Global::instance() -> errorOccurred(d->m_error);
        }
    }
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
    return d->m_error;
}
void QAsyncResult::set_error(QString v) {
    C_D(QAsyncResult);
    if (ycore::cmp_exchange(d->m_error, v)) {
        emit errorChanged();
    }
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
    return Global::instance()->qexecutor();
}

auto QAsyncResult::watch_dog() -> helper::WatchDog& {
    C_D(QAsyncResult);
    return d->m_wdog;
}

auto QAsyncResult::data() const -> QObject* {
    C_D(const QAsyncResult);
    return d->m_data;
}
void QAsyncResult::set_data(QObject* v) {
    C_D(QAsyncResult);
    if (ycore::cmp_exchange(d->m_data, v)) {
        dataChanged();
    }
    if (d->m_data != nullptr && d->m_data->parent() != this) {
        d->m_data->setParent(this);
    }
}

class ApiQuerierBase::Private {
public:
    Private(): m_auto_reload(true), m_dirty(true), m_session(nullptr) {}

    ~Private() {}

    bool m_auto_reload;
    bool m_qml_parsing;
    bool m_dirty;

    model::Session* m_session;
};

ApiQuerierBase::ApiQuerierBase(QObject* parent): QAsyncResult(parent), d_ptr(make_up<Private>()) {
    connect(this,
            &ApiQuerierBase::autoReloadChanged,
            this,
            &ApiQuerierBase::reload_if_needed,
            Qt::DirectConnection);
}

ApiQuerierBase::~ApiQuerierBase() {}

bool ApiQuerierBase::autoReload() const {
    C_D(const ApiQuerierBase);
    return d->m_auto_reload;
}

void ApiQuerierBase::set_autoReload(bool v) {
    C_D(ApiQuerierBase);
    if (std::exchange(d->m_auto_reload, v) != v) {
        emit autoReloadChanged();
    }
}

auto ApiQuerierBase::session() const -> model::Session* {
    C_D(const ApiQuerierBase);
    if (d->m_session == nullptr) {
        return Global::instance()->qsession();
    }
    return d->m_session;
}
void ApiQuerierBase::set_session(model::Session* val) {
    C_D(ApiQuerierBase);
    if (ycore::cmp_exchange(d->m_session, val)) {
        sessionChanged();
    }
}

void ApiQuerierBase::classBegin() {
    C_D(ApiQuerierBase);
    d->m_qml_parsing = true;
}
void ApiQuerierBase::componentComplete() {
    C_D(ApiQuerierBase);
    d->m_qml_parsing = false;
    reload_if_needed();
}

bool ApiQuerierBase::is_qml_parsing() const {
    C_D(const ApiQuerierBase);
    return d->m_qml_parsing;
}

bool ApiQuerierBase::dirty() const {
    C_D(const ApiQuerierBase);
    return d->m_dirty;
}
void ApiQuerierBase::mark_dirty(bool v) {
    C_D(ApiQuerierBase);
    d->m_dirty = v;
}

void ApiQuerierBase::reload_if_needed() {
    if (! is_qml_parsing() && autoReload() && dirty()) {
        reload();
        mark_dirty(false);
    }
}

void ApiQuerierBase::fetch_more(qint32) {}

void ApiQuerierBase::query() { reload(); }

} // namespace qcm