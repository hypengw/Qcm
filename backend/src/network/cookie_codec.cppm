export module qcm.backend:network.cookie_codec;
export import lifetchto;
export import :network.cookie_session;
using namespace rstd::prelude;

export namespace qcm::backend
{
auto decode_cookie_snapshot(ref<str> text) -> Option<Vec<lihttpto::StoredCookie>>;
auto encode_cookie_snapshot(slice<lihttpto::StoredCookie> cookies) -> Option<String>;
auto encode_cookie_snapshot(const Vec<lihttpto::StoredCookie>& cookies) -> Option<String>;

struct EncodedCookies {
    String      text;
    CookieStamp stamp;
};
auto encode_cookie_snapshot(const CookieSession& session) -> lifetchto::Result<EncodedCookies>;
} // namespace qcm::backend
