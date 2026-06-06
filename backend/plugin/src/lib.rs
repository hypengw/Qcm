use std::ops::Deref;

use qcm_core::global as qg;

pub fn init() {
    {
        use qcm_plugin_jellyfin::plugin::JellyfinPlugin;
        qg::add_plugin(Box::new(JellyfinPlugin::new()));
        use qcm_plugin_lua::plugin::LuaPlugin;
        qg::add_plugin(Box::new(LuaPlugin::new()));
    }

    let metas = qg::with_plugins(|plugins| {
        let mut metas = Vec::new();
        for p in plugins.values() {
            metas.extend(p.deref().provider_metas());
        }
        metas
    });

    for m in metas {
        qg::reg_provider_meta(m);
    }
}
