use mlua::prelude::*;
use qcm_core::plugin::{InstallationState, PluginContext};

pub(crate) struct LuaInstallation(pub PluginContext);

fn state_table(lua: &Lua, state: InstallationState) -> LuaResult<LuaTable> {
    let table = lua.create_table()?;
    table.set("revision", state.revision)?;
    if let Some(data) = state.data {
        table.set("data", data)?;
    }
    Ok(table)
}

impl LuaUserData for LuaInstallation {
    fn add_fields<F: LuaUserDataFields<Self>>(fields: &mut F) {
        fields.add_field_method_get("namespace", |_, this| Ok(this.0.namespace().to_owned()));
        fields.add_field_method_get("installation_id", |_, this| {
            Ok(this.0.installation_id().to_owned())
        });
        fields.add_field_method_get("runtime_id", |_, this| Ok(this.0.runtime_id().to_owned()));
        fields.add_field_method_get("platform", |lua, this| lua.to_value(this.0.platform()));
    }

    fn add_methods<M: LuaUserDataMethods<Self>>(methods: &mut M) {
        methods.add_async_function("load_state", |lua, ud: LuaAnyUserData| async move {
            let context = ud.borrow::<Self>()?.0.clone();
            let state = context.load_state().await.map_err(LuaError::external)?;
            state_table(&lua, state)
        });
        methods.add_async_function(
            "compare_exchange_state",
            |lua, (ud, revision, data): (LuaAnyUserData, LuaValue, LuaValue)| async move {
                let context = ud.borrow::<Self>()?.0.clone();
                let LuaValue::Integer(revision) = revision else {
                    return Err(LuaError::external(
                        "plugin state revision must be an integer",
                    ));
                };
                if revision < 0 {
                    return Err(LuaError::external(
                        "plugin state revision must be nonnegative",
                    ));
                }
                let LuaValue::String(data) = data else {
                    return Err(LuaError::external(
                        "plugin state data must be a UTF-8 string",
                    ));
                };
                let data = data.to_str()?.to_owned();
                let result = context
                    .compare_exchange_state(revision as u64, data)
                    .await
                    .map_err(LuaError::external)?;
                let table = lua.create_table()?;
                table.set("applied", result.applied)?;
                table.set("state", state_table(&lua, result.state)?)?;
                Ok(table)
            },
        );
    }
}
