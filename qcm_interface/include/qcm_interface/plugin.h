#pragma once

#include <QtCore/QtPlugin>

#include "qcm_interface/export.h"
#include "qcm_interface/router.h"
#include "qcm_interface/model/session.h"
#include "qcm_interface/model/plugin_info.h"

namespace qcm
{

class QCM_INTERFACE_API QcmPluginInterface {
public:
    QcmPluginInterface(std::string_view);
    virtual auto router() -> Router*                                    = 0;
    virtual auto info() -> const model::PluginInfo&                     = 0;
    virtual auto create_session() -> up<model::Session>                 = 0;
    virtual auto uniq(const QUrl& url, const QVariant& info) -> QString = 0;
};
class PluginModel;
class QCM_INTERFACE_API PluginManager {
    friend class PluginModel;

public:
    PluginManager();
    ~PluginManager();
    static auto instance() -> PluginManager*;
    void        register_plugin(std::string_view, qcm::QcmPluginInterface*);
    auto
        plugin(std::string_view) const -> std::optional<std::reference_wrapper<QcmPluginInterface>>;

private:
    class Private;
    C_DECLARE_PRIVATE(PluginManager, d_ptr);
};

} // namespace qcm

Q_DECLARE_INTERFACE(qcm::QcmPluginInterface, APP_ID ".PluginInterface")