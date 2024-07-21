#include "service_qml_ncm/api.h"

#include "qcm_interface/global.h"
#include "core/log.h"

using namespace qcm;

ncm::Client detail::get_client() {
    auto a = Global::instance()->client("ncm", []() -> std::any {
        return ncm::Client(Global::instance()->session(), Global::instance()->pool_executor());
    });
    return std::any_cast<ncm::Client>(a);
}

ApiQuerierBase::ApiQuerierBase(QObject* parent)
    : QObject(parent),
      m_main_ex(Global::instance()->qexecutor()),
      m_status(Status::Uninitialized),
      m_auto_reload(true),
      m_dirty(true),
      m_forward_error(true) {
    connect(this,
            &ApiQuerierBase::autoReloadChanged,
            this,
            &ApiQuerierBase::reload_if_needed,
            Qt::DirectConnection);
}

ApiQuerierBase::~ApiQuerierBase() {}

ApiQuerierBase::Status ApiQuerierBase::status() const { return m_status; }
void                   ApiQuerierBase::set_status(ApiQuerierBase::Status v) {
    if (m_status != v) {
        m_status = v;
        emit statusChanged();
        if (forwardError() && m_status == Status::Error) {
            emit Global::instance()->errorOccurred(m_error);
        }
    }
}

QString ApiQuerierBase::error() const { return m_error; }
void    ApiQuerierBase::set_error(QString v) {
    if (m_error != v) {
        m_error = v;
        emit errorChanged();
    }
}

bool ApiQuerierBase::autoReload() const { return m_auto_reload; }
void ApiQuerierBase::set_autoReload(bool v) {
    if (std::exchange(m_auto_reload, v) != v) {
        emit autoReloadChanged();
    }
}

bool ApiQuerierBase::forwardError() const { return m_forward_error; }
void ApiQuerierBase::set_forwardError(bool v) {
    if (std::exchange(m_forward_error, v) != v) {
        emit forwardErrorChanged();
    }
}

void ApiQuerierBase::classBegin() { m_qml_parsing = true; }
void ApiQuerierBase::componentComplete() {
    m_qml_parsing = false;
    reload_if_needed();
}

bool ApiQuerierBase::is_qml_parsing() const { return m_qml_parsing; }

bool ApiQuerierBase::dirty() const { return m_dirty; }
void ApiQuerierBase::mark_dirty(bool v) { m_dirty = v; }
