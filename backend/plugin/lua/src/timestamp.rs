use mlua::prelude::*;
use qcm_core::db::values::Timestamp;

pub fn create_time_module(lua: &Lua) -> LuaResult<LuaTable> {
    let exports = lua.create_table()?;

    exports.set(
        "now",
        lua.create_function(|_lua, (): ()| Ok(Timestamp::now().as_millis()))?,
    )?;

    Ok(exports)
}
