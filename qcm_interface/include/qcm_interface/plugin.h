#pragma once

#include <QtCore/QtPlugin>

#include "qcm_interface/export.h"
#include "qcm_interface/router.h"
#include "qcm_interface/model/page.h"
#include "qcm_interface/model/plugin_info.h"

namespace qcm
{
class QCM_INTERFACE_API QcmPluginInterface {
public:
    virtual auto router() -> Router*                      = 0;
    virtual auto info() -> const model::PluginInfo&       = 0;
    virtual auto main_pages() -> std::vector<model::Page> = 0;
};

} // namespace qcm

Q_DECLARE_INTERFACE(qcm::QcmPluginInterface, APP_ID ".PluginInterface")