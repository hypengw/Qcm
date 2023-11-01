#include "Qcm/api.h"

#include "Qcm/app.h"
#include "core/log.h"

using namespace qcm;

ncm::Client detail::get_client() { return App::instance()->ncm_client(); }

ApiQuerierBase::ApiQuerierBase(QObject* parent)
    : QObject(parent),
      m_main_ex(App::instance()->get_executor()),
      m_status(Status::Uninitialized),
      m_auto_reload(true),
      m_dirty(true) {
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
        if (m_status == Status::Error) {
            emit App::instance()->errorOccurred(m_error);
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

bool ApiQuerierBase::auto_reload() const { return m_auto_reload; }
void ApiQuerierBase::set_auto_reload(bool v) {
    if (m_auto_reload != v) {
        m_auto_reload = v;
        emit autoReloadChanged();
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
