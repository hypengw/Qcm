#include <QtCore/qtsymbolmacros.h>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqmlengine.h>

#include "service_qml_ncm/ncm_image.h"

QT_DECLARE_EXTERN_SYMBOL_VOID(qml_register_types_Qcm_Service_Ncm);
class Qcm_Service_NcmPlugin : public QQmlEngineExtensionPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)

public:
    Qcm_Service_NcmPlugin(QObject* parent = nullptr): QQmlEngineExtensionPlugin(parent) {
        QT_KEEP_SYMBOL(qml_register_types_Qcm_Service_Ncm);
    }

    void initializeEngine(QQmlEngine* engine, const char* uri) {
        Q_UNUSED(uri);
        auto p = qcm::qml_ncm::create_ncm_imageprovider();
        engine->addImageProvider(u"ncm"_qs, p);
    }
};

#include "plugin.moc"