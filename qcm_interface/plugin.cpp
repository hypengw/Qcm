#include "qcm_interface/plugin.h"
#include "qcm_interface/plugin_p.h"

namespace qcm
{

namespace
{
auto the_plugin_manager() -> PluginManager*;
} // namespace

QcmPluginInterface::QcmPluginInterface(std::string_view name) {
    the_plugin_manager()->register_plugin(name, this);
}

auto PluginManager::instance() -> PluginManager* { return the_plugin_manager(); }
PluginManager::PluginManager(): d_ptr(make_up<Private>()) {}
PluginManager::~PluginManager() {}
void PluginManager::register_plugin(std::string_view name, qcm::QcmPluginInterface* plugin) {
    C_D(PluginManager);
    d->plugins.insert({ std::string(name), plugin });
}
auto PluginManager::plugin(std::string_view name) const
    -> std::optional<std::reference_wrapper<QcmPluginInterface>> {
    C_D(const PluginManager);
    if (auto it = d->plugins.find(name); it != d->plugins.end()) {
        return *(it->second);
    }
    return std::nullopt;
}

namespace
{
auto the_plugin_manager() -> PluginManager* {
    static PluginManager m;
    return &m;
}

} // namespace

} // namespace qcm