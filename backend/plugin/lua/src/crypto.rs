use mlua::prelude::*;
use qcm_core::crypto::*;
use serde_urlencoded;

struct LuaRsa(Rsa<pkey::Public>);

impl LuaUserData for LuaRsa {
    fn add_methods<M: LuaUserDataMethods<Self>>(methods: &mut M) {
        methods.add_method("encrypt", |lua, this, data: LuaString| {
            let mut enc_data = vec![0; this.0.size() as usize];
            let enc_len = this
                .0
                .public_encrypt(&data.as_bytes(), &mut enc_data, Padding::NONE)
                .map_err(|e| LuaError::RuntimeError(e.to_string()))?;

            enc_data.truncate(enc_len);
            Ok(lua.create_string(enc_data))
        });
    }
}

fn get_cipher(cipher_name: &str) -> Option<Cipher> {
    match cipher_name {
        "aes-128-cbc" => Some(Cipher::aes_128_cbc()),
        "aes-128-ecb" => Some(Cipher::aes_128_ecb()),
        _ => None,
    }
}

pub fn create_crypto_module(lua: &Lua) -> LuaResult<LuaTable> {
    let exports = lua.create_table()?;

    exports.set(
        "encrypt",
        lua.create_function(
            |lua, (cipher_name, key, iv, data): (String, LuaString, LuaString, LuaString)| {
                let cipher = get_cipher(cipher_name.as_str())
                    .ok_or_else(|| LuaError::RuntimeError("Unsupported cipher".to_string()))?;
                encrypt(cipher, &key.as_bytes(), &iv.as_bytes(), &data.as_bytes())
                    .map_err(|e| LuaError::RuntimeError(e.to_string()))
                    .map(|b| lua.create_string(b))
            },
        )?,
    )?;

    exports.set(
        "decrypt",
        lua.create_function(
            |lua, (cipher_name, key, iv, data): (String, LuaString, LuaString, LuaString)| {
                let cipher = get_cipher(cipher_name.as_str())
                    .ok_or_else(|| LuaError::RuntimeError("Unsupported cipher".to_string()))?;
                decrypt(cipher, &key.as_bytes(), &iv.as_bytes(), &data.as_bytes())
                    .map_err(|e| LuaError::RuntimeError(e.to_string()))
                    .map(|b| lua.create_string(b))
            },
        )?,
    )?;

    exports.set(
        "encode",
        lua.create_function(|lua, data: LuaString| {
            encode(&data.as_bytes())
                .map_err(|e| LuaError::RuntimeError(e.to_string()))
                .map(|b| lua.create_string(b))
        })?,
    )?;

    exports.set(
        "encode_block",
        lua.create_function(|_, data: LuaString| Ok(encode_block(&data.as_bytes())))?,
    )?;

    exports.set(
        "decode",
        lua.create_function(|lua, data: LuaString| {
            decode(&data.as_bytes())
                .map_err(|e| LuaError::RuntimeError(e.to_string()))
                .map(|b| lua.create_string(b))
        })?,
    )?;

    exports.set(
        "digest",
        lua.create_function(|lua, (digest_name, data): (String, LuaString)| {
            let digest_type = match digest_name.as_str() {
                "md5" => MessageDigest::md5(),
                "sha1" => MessageDigest::sha1(),
                _ => {
                    return Err(LuaError::RuntimeError(
                        "Unsupported digest type".to_string(),
                    ))
                }
            };
            digest(digest_type, &data.as_bytes())
                .map_err(|e| LuaError::RuntimeError(e.to_string()))
                .map(|b| lua.create_string(b))
        })?,
    )?;

    let hex = lua.create_table()?;
    hex.set(
        "encode_low",
        lua.create_function(|lua, data: LuaString| {
            Ok(lua.create_string(hex::encode_low(&data.as_bytes())))
        })?,
    )?;
    hex.set(
        "encode_up",
        lua.create_function(|lua, data: LuaString| {
            Ok(lua.create_string(hex::encode_up(&data.as_bytes())))
        })?,
    )?;
    exports.set("hex", hex)?;

    let url = lua.create_table()?;
    url.set(
        "encode",
        lua.create_function(|_, v: LuaValue| {
            serde_urlencoded::to_string(&v).map_err(mlua::Error::external)
        })?,
    )?;
    exports.set("url", url)?;

    exports.set(
        "create_rsa",
        lua.create_function(|_, pub_key: LuaString| {
            let rsa = Rsa::public_key_from_pem(&pub_key.as_bytes())
                .map_err(|e| LuaError::RuntimeError(e.to_string()))?;
            Ok(LuaRsa(rsa))
        })?,
    )?;

    Ok(exports)
}
