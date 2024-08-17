#include "qcm_interface/api.h"
#include "qcm_interface/global.h"

namespace qcm
{

class ApiQuerierBase::Private {
public:
    Private()
        : m_main_ex(Global::instance()->qexecutor()),
          m_status(Status::Uninitialized),
          m_auto_reload(true),
          m_dirty(true),
          m_forward_error(true) {}

    ~Private() {}

    QtExecutor       m_main_ex;
    helper::WatchDog m_wdog;
    Status           m_status;
    QString          m_error;
    bool             m_auto_reload;
    bool             m_qml_parsing;
    bool             m_dirty;
    bool             m_forward_error;
};

ApiQuerierBase::ApiQuerierBase(QObject* parent): QObject(parent), d_ptr(make_up<Private>()) {
    connect(this,
            &ApiQuerierBase::autoReloadChanged,
            this,
            &ApiQuerierBase::reload_if_needed,
            Qt::DirectConnection);
}

ApiQuerierBase::~ApiQuerierBase() {}

ApiQuerierBase::Status ApiQuerierBase::status() const {
    C_D(const ApiQuerierBase);
    return d->m_status;
}
void ApiQuerierBase::set_status(ApiQuerierBase::Status v) {
    C_D(ApiQuerierBase);
    if (d->m_status != v) {
        d->m_status = v;
        emit statusChanged();
        if (forwardError() && d->m_status == Status::Error) {
            emit Global::instance() -> errorOccurred(d->m_error);
        }
    }
}

auto ApiQuerierBase::error() const -> const QString& {
    C_D(const ApiQuerierBase);
    return d->m_error;
}
void ApiQuerierBase::set_error(QString v) {
    C_D(ApiQuerierBase);
    if (d->m_error != v) {
        d->m_error = v;
        emit errorChanged();
    }
}

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

bool ApiQuerierBase::forwardError() const {
    C_D(const ApiQuerierBase);
    return d->m_forward_error;
}
void ApiQuerierBase::set_forwardError(bool v) {
    C_D(ApiQuerierBase);
    if (std::exchange(d->m_forward_error, v) != v) {
        emit forwardErrorChanged();
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
void ApiQuerierBase::cancel() {
    C_D(ApiQuerierBase);
    d->m_wdog.cancel();
}
auto ApiQuerierBase::get_executor() -> QtExecutor& {
    C_D(ApiQuerierBase);
    return d->m_main_ex;
}

auto ApiQuerierBase::watch_dog() -> helper::WatchDog& {
    C_D(ApiQuerierBase);
    return d->m_wdog;
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