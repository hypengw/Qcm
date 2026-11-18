module;
#include <rstd/enum.hpp>
#include <rstd/macro.hpp>
module qcm.backend;
import :network.cookie_codec;
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

struct DomainView {
    const lihttpto::StoredCookie& cookie;
};
struct DomainViewDefinition {
    static constexpr bool serializable   = true;
    static constexpr bool deserializable = false;
    template<class S>
    static auto serialize(S& sink, const DomainView& value) -> typename S::result_type {
        return sink.serialize_newtype_variant(
            value.cookie.host_only ? "HostOnly"_str : "Suffix"_str, value.cookie.domain.as_str());
    }
};
constexpr auto rstd_serde_definition(DomainView*) { return DomainViewDefinition {}; }

struct ExpirationView {
    const lihttpto::StoredCookie& cookie;
};
struct ExpirationViewDefinition {
    static constexpr bool serializable   = true;
    static constexpr bool deserializable = false;
    template<class S>
    static auto serialize(S& sink, const ExpirationView& value) -> typename S::result_type {
        const auto& cookie = value.cookie;
        if (cookie.expires.is_none()) return sink.serialize_unit_variant("SessionEnd"_str);
        auto text = format_utc_datetime(UtcDateTime { *cookie.expires, cookie.expiry_nanoseconds });
        if (text.is_none()) return Err(sink.invalid_value("invalid cookie expiry"_str));
        return sink.serialize_newtype_variant("AtUtc"_str, text->as_str());
    }
};
constexpr auto rstd_serde_definition(ExpirationView*) { return ExpirationViewDefinition {}; }

struct CookieView {
    const lihttpto::StoredCookie& cookie;
};
struct CookieViewDefinition {
    static constexpr bool serializable   = true;
    static constexpr bool deserializable = false;
    template<class S>
    static auto serialize(S& sink, const CookieView& value) -> typename S::result_type {
        const auto& cookie = value.cookie;
        auto        output = rstd_try(sink.begin_map(usize(4)));
        rstd_try(rstd::serde::field(output, "domain"_str, DomainView { cookie }));
        rstd_try(rstd::serde::field(output, "expires"_str, ExpirationView { cookie }));
        rstd_try(rstd::serde::field(
            output, "path"_str, rstd::tuple { cookie.path.as_str(), cookie.path_explicit }));
        if (cookie.raw_cookie.is_empty()) {
            auto raw = cookie.cookie.encode();
            if (cookie.secure) raw.push_str("; Secure"_str);
            if (cookie.http_only) raw.push_str("; HttpOnly"_str);
            rstd_try(rstd::serde::field(output, "raw_cookie"_str, raw.as_str()));
        } else {
            rstd_try(rstd::serde::field(output, "raw_cookie"_str, cookie.raw_cookie.as_str()));
        }
        return output.end();
    }
};
constexpr auto rstd_serde_definition(CookieView*) { return CookieViewDefinition {}; }

struct SnapshotView {
    slice<lihttpto::StoredCookie> cookies;
};
struct SnapshotViewDefinition {
    static constexpr bool serializable   = true;
    static constexpr bool deserializable = false;
    template<class S>
    static auto serialize(S& sink, const SnapshotView& value) -> typename S::result_type {
        auto output = rstd_try(sink.begin_sequence(value.cookies.len()));
        for (const auto& cookie : value.cookies) rstd_try(output.element(CookieView { cookie }));
        return output.end();
    }
};
constexpr auto rstd_serde_definition(SnapshotView*) { return SnapshotViewDefinition {}; }
} // namespace qcm::backend::cookie_wire

namespace qcm::backend
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

auto encode_cookie_snapshot(slice<lihttpto::StoredCookie> cookies) -> Option<String> {
    auto encoded = rstd::json::encode_direct(cookie_wire::SnapshotView { cookies });
    if (encoded.is_err()) return None();
    return Some(rstd::move(*encoded));
}

auto encode_cookie_snapshot(const Vec<lihttpto::StoredCookie>& cookies) -> Option<String> {
    return encode_cookie_snapshot(cookies.as_slice());
}

auto encode_cookie_snapshot(const CookieSession& session) -> lifetchto::Result<EncodedCookies> {
    Option<EncodedCookies> encoded;
    auto visitor = [&](slice<lihttpto::StoredCookie> cookies, Option<u64> revision) {
        if (auto text = encode_cookie_snapshot(cookies))
            encoded =
                Some(EncodedCookies { rstd::move(*text), CookieStamp(session.clone(), revision) });
    };
    rstd_try(session.with_snapshot(dyn<lifetchto::http::CookieSnapshotVisitor>::from_ref(visitor)));
    if (encoded.is_none())
        return Err(lifetchto::Error::InvalidOptions("invalid cookie expiry"_Str));
    return Ok(rstd::move(*encoded));
}
} // namespace qcm::backend
