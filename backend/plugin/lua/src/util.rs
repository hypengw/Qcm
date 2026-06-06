use mlua::prelude::*;
use serde::Serialize;

pub fn to_lua<T>(l: &Lua, t: &T) -> LuaResult<LuaValue>
where
    T: Serialize + ?Sized,
{
    l.to_value_with(
        &t,
        LuaSerializeOptions::new()
            .serialize_none_to_null(false)
            .serialize_unit_to_null(false),
    )
}
