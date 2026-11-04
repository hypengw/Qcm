use super::LuaProviderInner;
use qcm_core::{
    http::{CookieStoreTrait, HeaderValue},
    provider::ProviderCommonData,
};
use std::sync::{
    atomic::{AtomicI64, AtomicU64, Ordering},
    Arc, RwLock,
};

#[test]
fn http_client_rejects_stale_generation_after_network_replacement() {
    let inner = LuaProviderInner {
        common: ProviderCommonData::new(None, "synthetic", "seed", "synthetic"),
        network: RwLock::new(crate::session::Network::new().unwrap()),
        session: crate::session::Session::new(),
        tasks: Default::default(),
        generation: Arc::new(AtomicU64::new(1)),
        network_scope: AtomicI64::new(0),
    };
    let expected = inner.generation.load(Ordering::Acquire);
    let url = reqwest::Url::parse("https://example.invalid/").unwrap();
    let next = crate::session::Network::new().unwrap();
    let headers = [HeaderValue::from_static("session=current; Path=/; Secure")];
    next.jar.set_cookies(&mut headers.iter(), &url);

    {
        let mut network = inner.network.write().unwrap();
        network.invalidate();
        *network = next;
        inner.generation.fetch_add(1, Ordering::AcqRel);
    }

    assert!(inner.http_client(expected).is_err());
    assert!(inner.http_client(expected + 1).is_ok());
    let network = inner.network.read().unwrap();
    assert_eq!(
        network.jar.cookies(&url).unwrap().to_str().unwrap(),
        "session=current"
    );
}
