module;
#include <rstd/enum.hpp>
export module qcm.backend:network.cookie_codec;
export import lifetchto;
import rstd.serde;
import rstd.json;
import :datetime;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend::cookie_wire
{
class Domain {
    RSTD_ENUM(Domain, (HostOnly, (String value;)), (Suffix, (String value;)))
};
constexpr auto rstd_serde_definition(Domain::HostOnly_payload*) {
    using T = Domain::HostOnly_payload;
    return rstd::serde::
        NewtypeDefinition<T, "HostOnly", rstd::serde::Direction::Both, &T::value> {};
}
constexpr auto rstd_serde_definition(Domain::Suffix_payload*) {
    using T = Domain::Suffix_payload;
    return rstd::serde::NewtypeDefinition<T, "Suffix", rstd::serde::Direction::Both, &T::value> {};
}
constexpr auto rstd_serde_definition(Domain*) {
    return rstd::serde::VariantDefinition<Domain, "CookieDomain", rstd::serde::Direction::Both> {};
}
class Expiration {
    RSTD_ENUM(Expiration, (SessionEnd), (AtUtc, (String value;)))
};
constexpr auto rstd_serde_definition(Expiration::AtUtc_payload*) {
    using T = Expiration::AtUtc_payload;
    return rstd::serde::NewtypeDefinition<T, "AtUtc", rstd::serde::Direction::Both, &T::value> {};
}
constexpr auto rstd_serde_definition(Expiration*) {
    return rstd::serde::
        VariantDefinition<Expiration, "CookieExpiration", rstd::serde::Direction::Both> {};
}
struct Cookie {
    String                    raw_cookie;
    Domain                    domain { Domain::HostOnly(String {}) };
    rstd::tuple<String, bool> path;
    Expiration                expires { Expiration::SessionEnd() };
};
constexpr auto rstd_serde_definition(Cookie*) {
    using namespace rstd::serde;
    // Preserve the persisted object's key order.
    return RecordDefinition<Cookie,
                            "Cookie",
                            Direction::Both,
                            UnknownFieldPolicy::Ignore,
                            Member<&Cookie::domain, "domain">,
                            Member<&Cookie::expires, "expires">,
                            Member<&Cookie::path, "path">,
                            Member<&Cookie::raw_cookie, "raw_cookie">> {};
}
} // namespace qcm::backend::cookie_wire

export namespace qcm::backend
{
auto decode_cookie_snapshot(ref<str> text) -> Option<Vec<lihttpto::StoredCookie>> {
    auto parsed = rstd::json::decode<Vec<cookie_wire::Cookie>>(text);
    if (parsed.is_err()) return None();
    Vec<lihttpto::StoredCookie> result;
    for (auto& item : *parsed) {
        auto cookie = lihttpto::SetCookie::parse(item.raw_cookie.as_str());
        if (cookie.is_err()) return None();
        Option<i64> seconds;
        unsigned    nanos {};
        if (item.expires.is_AtUtc()) {
            auto time = parse_utc_datetime(item.expires.as_AtUtc().value.as_str());
            if (time.is_none() || time->nanoseconds >= 1000000000U) return None();
            seconds = Some(time->seconds);
            nanos   = time->nanoseconds;
        }
        auto host_only = item.domain.is_HostOnly();
        auto domain    = host_only ? rstd::move(item.domain.as_HostOnly().value)
                                   : rstd::move(item.domain.as_Suffix().value);
        result.push({ cookie->cookie().clone(),
                      rstd::move(domain),
                      rstd::move(rstd::get<0>(item.path)),
                      seconds,
                      host_only,
                      cookie->secure(),
                      cookie->http_only(),
                      rstd::move(item.raw_cookie),
                      rstd::get<1>(item.path),
                      nanos });
    }
    return Some(rstd::move(result));
}
auto encode_cookie_snapshot(const Vec<lihttpto::StoredCookie>& cookies) -> Option<String> {
    Vec<cookie_wire::Cookie> items;
    for (const auto& cookie : cookies.as_slice()) {
        auto raw = cookie.raw_cookie.clone();
        if (raw.is_empty()) {
            raw = cookie.cookie.encode();
            if (cookie.secure) raw.push_str("; Secure"_str);
            if (cookie.http_only) raw.push_str("; HttpOnly"_str);
        }
        auto domain = cookie.host_only ? cookie_wire::Domain::HostOnly(cookie.domain.clone())
                                       : cookie_wire::Domain::Suffix(cookie.domain.clone());
        auto expiry = cookie_wire::Expiration::SessionEnd();
        if (cookie.expires.is_some()) {
            auto text =
                format_utc_datetime(UtcDateTime { *cookie.expires, cookie.expiry_nanoseconds });
            if (text.is_none()) return None();
            expiry = cookie_wire::Expiration::AtUtc(rstd::move(*text));
        }
        items.push({ rstd::move(raw),
                     rstd::move(domain),
                     rstd::tuple { cookie.path.clone(), cookie.path_explicit },
                     rstd::move(expiry) });
    }
    auto encoded = rstd::json::encode_direct(items);
    if (encoded.is_err()) return None();
    return Some(rstd::move(*encoded));
}
} // namespace qcm::backend
