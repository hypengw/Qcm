pub use bytes::Bytes;
pub use reqwest::cookie::CookieStore as CookieStoreTrait;
pub use reqwest::header::{HeaderMap, HeaderValue};
pub use reqwest::Client as HttpClient;
pub use reqwest::ClientBuilder as HttpClientBuilder;
pub use reqwest_cookie_store::{CookieStore, CookieStoreRwLock};

use crate::provider::ProviderSession;
use log;
use std::io::Cursor;
use std::sync::Arc;

pub trait HasCookieJar {
    fn jar(&self) -> Arc<CookieStoreRwLock>;
}

pub fn load_cookie_jar(jar: Arc<CookieStoreRwLock>, data: &str) {
    match cookie_store::serde::json::load_all(Cursor::new(data)) {
        Ok(loaded) => {
            let mut jar = jar.write().unwrap();
            jar.clear();
            *jar = loaded;
        }
        Err(e) => {
            log::error!("Failed to parse cookie data: {}", e);
        }
    }
}
pub fn save_cookie_jar(jar: Arc<CookieStoreRwLock>) -> String {
    let jar = jar.read().unwrap();
    let mut cursor = Cursor::new(Vec::new());
    cookie_store::serde::json::save_incl_expired_and_nonpersistent(&jar, &mut cursor)
        .expect("Failed to save cookies to string");

    String::from_utf8(cursor.into_inner()).unwrap_or_default()
}

impl<T: HasCookieJar> ProviderSession for T {
    fn load_cookie(&self, data: &str) {
        load_cookie_jar(self.jar(), data);
    }

    fn save_cookie(&self) -> String {
        save_cookie_jar(self.jar())
    }
}

pub fn client_builder_with_jar(jar: Arc<CookieStoreRwLock>) -> HttpClientBuilder {
    HttpClientBuilder::new().cookie_provider(jar)
}
