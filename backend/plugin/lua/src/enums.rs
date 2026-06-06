use mlua::prelude::*;
use qcm_core::event::SyncState;
use qcm_core::model::type_enum::{AlbumType, ItemType};
use qcm_core::IntoEnumIterator;
use std::fmt::Display;

pub fn register_enum<T>(lua: &Lua) -> LuaResult<LuaTable>
where
    T: IntoEnumIterator + Into<i32> + Display + Copy,
{
    let enum_table = lua.create_table()?;
    for value in T::iter() {
        let v_i32: i32 = value.into();
        let key = value.to_string();
        enum_table.set(key, v_i32)?;
    }
    Ok(enum_table)
}

pub fn create_module(lua: &Lua) -> LuaResult<LuaTable> {
    let t = lua.create_table()?;
    t.set("ItemType", register_enum::<ItemType>(lua)?)?;
    t.set("SyncState", register_enum::<SyncState>(lua)?)?;
    t.set("AlbumType", register_enum::<AlbumType>(lua)?)?;
    Ok(t)
}
