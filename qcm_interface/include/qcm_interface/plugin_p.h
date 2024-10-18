#pragma once

#include "qcm_interface/plugin.h"

namespace qcm
{

class PluginManager::Private {
public:
    std::map<std::string, QcmPluginInterface*, std::less<>> plugins;
};

} // namespace qcm