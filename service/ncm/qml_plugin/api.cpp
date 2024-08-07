#include "service_qml_ncm/api.h"

#include "qcm_interface/global.h"
#include "service_qml_ncm/model.h"
#include "core/log.h"

namespace ncm::impl
{

static auto server_url(std::any& client, const qcm::model::ItemId& id) -> std::string {
    auto c = std::any_cast<ncm::Client>(&client);
    switch (ncm_id_type(id)) {
    case ncm::model::IdType::Song: {
        return fmt::format("{}/#song?id={}", ncm::BASE_URL, id.id());
    }
    default: {
        return {};
    }
    }
}

} // namespace ncm::impl

namespace qcm
{

ncm::Client detail::get_client() {
    auto a = Global::instance()->client("ncm", []() -> Global::Client {
        auto api        = make_rc<Global::Client::Api>();
        api->server_url = ncm::impl::server_url;
        return { .api      = api,
                 .instance = ncm::Client(Global::instance()->session(),
                                         Global::instance()->pool_executor()) };
    });
    return std::any_cast<ncm::Client>(a.instance);
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

} // namespace qcm