#include "Qcm/app.h"

#include <QQuickWindow>
#include <QQuickStyle>
#include <QGlobalStatic>
#include <qapplicationstatic.h>
#include <QSettings>

#include <asio/deferred.hpp>

#include "core/path.h"
#include "Qcm/type.h"
#include "Qcm/ncm_image.h"
#include "Qcm/qr_image.h"
#include "ncm/api/user_account.h"
#include "crypto/crypto.h"
#include "Qcm/info.h"

using namespace qcm;

Q_APPLICATION_STATIC(App, app);

App* App::instance() { return app; }

App::App()
    : QObject(nullptr),
      m_qt_ex(std::make_shared<QtExecutionContext>(this)),
      m_pool(4),
      m_session(std::make_shared<request::Session>(m_pool.get_executor())),
      m_client(m_session, m_pool.get_executor()) {
    QGuiApplication::setApplicationName(AppName.data());
    QGuiApplication::setOrganizationName(AppName.data());
    // QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);
}
App::~App() {
    save_session();
    m_pool.join();
}

ncm::Client App::ncm_client() const { return m_client; }

void App::init(QQmlApplicationEngine* engine) {
    qmlRegisterSingletonInstance("QcmApp", 1, 0, "App", this);
    qcm::init_path(std::array { config_path() / "session", data_path() });

    load_session();

    QQuickStyle::setStyle("Material");

    engine->addImageProvider(u"ncm"_qs, new NcmImageProvider {});
    engine->addImageProvider(u"qr"_qs, new QrImageProvider {});

    engine->load(u"qrc:/QcmApp/main.qml"_qs);
}

model::ArtistId App::artistId(QString id) const { return { id }; }
model::AlbumId  App::albumId(QString id) const { return { id }; }

QString App::md5(QString txt) const {
    auto opt = crypto::digest(crypto::md5(), To<std::vector<byte>>::from(txt.toStdString()))
                   .map([](auto in) {
                       return To<QString>::from(crypto::hex::encode_low(in));
                   });
    _assert_(opt);
    return std::move(opt).value();
}

void App::loginPost(model::UserAccount* user) {
    auto& id = user->m_userId;
    if (id.valid()) {
        QSettings s;
        s.setValue("session/user_id", To<QString>::from((fmt::format("ncm-{}", id.id))));

        save_session();
    }
}

void App::load_session() {
    QSettings s;
    auto      user_id = To<std::string>::from(s.value("session/user_id").toString());
    if (! user_id.empty()) {
        m_session->load_cookie(config_path() / "session" / user_id);
    }
}

void App::save_session() {
    QSettings s;
    auto      user_id = To<std::string>::from(s.value("session/user_id").toString());
    if (! user_id.empty()) {
        m_session->save_cookie(config_path() / "session" / user_id);
    }
}
