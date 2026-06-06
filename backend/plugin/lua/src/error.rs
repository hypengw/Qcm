use mlua::Lua;
use qcm_core::error::ProviderError;
use std::sync::Arc;

pub trait FromLuaError<F> {
    fn from_err_ref(err: &F) -> ProviderError;
    fn from_err(err: F) -> ProviderError {
        Self::from_err_ref(&err)
    }
}

impl FromLuaError<mlua::Error> for ProviderError {
    fn from_err_ref(err: &mlua::Error) -> ProviderError {
        match &err {
            mlua::Error::ExternalError(err) => match err.downcast_ref::<ProviderError>() {
                Some(ProviderError::Lua(str)) => ProviderError::Lua(str.clone()),
                _ => ProviderError::External(Arc::new(err.clone())),
            },
            mlua::Error::CallbackError { traceback, cause } => ProviderError::WithContext {
                context: traceback.clone(),
                err: Arc::new(ProviderError::from_err_ref(&cause)),
            },
            mlua::Error::WithContext { context, cause } => ProviderError::WithContext {
                context: context.clone(),
                err: Arc::new(ProviderError::from_err_ref(cause)),
            },
            err => ProviderError::Lua(err.to_string()),
        }
    }
}

pub fn create_lua_error_func(lua: &Lua) -> Result<mlua::Function, mlua::Error> {
    lua.create_function(|_, v: mlua::Value| match v {
        mlua::Value::String(s) => Err(mlua::Error::external(ProviderError::Lua(
            s.to_str()?.to_owned(),
        ))),
        _ => Ok(()),
    })
}
