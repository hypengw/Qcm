#include <QtCore/qtsymbolmacros.h>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqmlengine.h>

#include "qcm_interface/plugin.h"
#include "qcm_interface/enum.h"
#include "qcm_interface/model/page.h"
#include "service_qml_ncm/client.h"
#include "service_qml_ncm/session.h"
#include "core/log.h"

#define QRC_URL  u"qrc:/Qcm/Service/Ncm/"
#define PAGE_URL QRC_URL u"qml/page/"

QT_DECLARE_EXTERN_SYMBOL_VOID(qml_register_types_Qcm_Service_Ncm);
class Qcm_Service_NcmPlugin : public QQmlEngineExtensionPlugin, public qcm::QcmPluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)
    Q_INTERFACES(qcm::QcmPluginInterface)

public:
    Qcm_Service_NcmPlugin(QObject* parent = nullptr)
        : QQmlEngineExtensionPlugin(parent),
          qcm::QcmPluginInterface(ncm::qml::provider),
          m_router(new qcm::Router(this)) {
        QT_KEEP_SYMBOL(qml_register_types_Qcm_Service_Ncm);
        m_router->register_path(qcm::enums::PluginBasicPage::BPageLogin, PAGE_URL u"LoginPage.qml");

        m_info.set_name("ncm");
        m_info.set_fullname("Netease Cloud Music");
        m_info.set_icon(QUrl(QStringLiteral(QRC_URL u"assets/netease.svg")));
    }

    void initializeEngine(QQmlEngine* engine, const char* uri) override {
        Q_UNUSED(engine);
        Q_UNUSED(uri);
        // auto p = qcm::qml_ncm::create_ncm_imageprovider();
        // engine->addImageProvider(u"ncm"_qs, p);
    }

    auto router() -> qcm::Router* override { return m_router; }
    auto info() -> const qcm::model::PluginInfo& override { return m_info; }

    virtual auto create_session() -> up<qcm::model::Session> override {
        return make_up<ncm::qml::Session>();
    }

    virtual auto uniq(const QUrl& url, const QVariant& info) -> QString override {
        return ncm::qml::uniq(url, info);
    }

private:
    qcm::Router*           m_router;
    qcm::model::PluginInfo m_info;
};

#include "plugin.moc"