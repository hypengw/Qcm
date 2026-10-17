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
                Some(ProviderError::NotAuth) => ProviderError::NotAuth,
                Some(ProviderError::AuthRequired { message, jump_url }) => {
                    ProviderError::AuthRequired {
                        message: message.clone(),
                        jump_url: jump_url.clone(),
                    }
                }
                Some(ProviderError::NotFound) => ProviderError::NotFound,
                Some(ProviderError::NotImplemented) => ProviderError::NotImplemented,
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
        mlua::Value::Table(table) => {
            let kind: Option<String> = table.get("code")?;
            let status: Option<i32> = table.get("status")?;
            let error = match (kind.as_deref(), status) {
                (Some("not_auth"), _) | (_, Some(401)) => {
                    let message = table.get::<Option<String>>("message")?;
                    let jump_url = table.get::<Option<String>>("jump_url")?.filter(|url| {
                        reqwest::Url::parse(url).is_ok_and(|u| {
                            matches!(u.scheme(), "http" | "https")
                                && u.username().is_empty()
                                && u.password().is_none()
                        })
                    });
                    if message.is_some() || jump_url.is_some() {
                        ProviderError::AuthRequired {
                            message: message.unwrap_or_else(|| "Login required".into()),
                            jump_url,
                        }
                    } else {
                        ProviderError::NotAuth
                    }
                }
                (Some("not_found"), _) | (_, Some(404)) => ProviderError::NotFound,
                (Some("not_implemented"), _) => ProviderError::NotImplemented,
                _ => ProviderError::Lua(format!(
                    "{}: {}",
                    kind.unwrap_or_else(|| "error".into()),
                    table.get::<Option<String>>("message")?.unwrap_or_default()
                )),
            };
            Err(mlua::Error::external(error))
        }
        _ => Ok(()),
    })
}
