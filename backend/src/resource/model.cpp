module;
#include <rstd/macro.hpp>
module qcm.backend;
import :resource.model;
using rstd::time::Duration;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend
{
auto resource_header_allowed(const lihttpto::HeaderName& name) -> bool {
    static constexpr ref<str> allowed[] = { "accept"_str,           "accept-language"_str,
                                            "icy-metadata"_str,     "cache-control"_str,
                                            "pragma"_str,           "range"_str,
                                            "if-range"_str,         "if-none-match"_str,
                                            "if-modified-since"_str };
    for (auto field : allowed)
        if (name.matches(field)) return true;
    return false;
}
} // namespace qcm::backend

namespace qcm::backend::resource_cache
{
PreparedRequest::PreparedRequest(lihttpto::Url url, String cache_url, lihttpto::Headers headers,
                                 lihttpto::Headers delivery, Option<Duration> timeout,
                                 CacheMode mode, Vec<String> transient)
    : url_(rstd::move(url)),
      cache_url_(rstd::move(cache_url)),
      headers_(rstd::move(headers)),
      delivery_headers_(rstd::move(delivery)),
      timeout_(timeout),
      mode_(mode),
      transient_query_(rstd::move(transient)) {}

auto PreparedRequest::clear_conditions(lihttpto::Headers& headers) -> void {
    for (auto name : { "range"_str, "if-range"_str, "if-none-match"_str, "if-modified-since"_str })
        headers.remove(name);
}

auto PreparedRequest::normalized_url(const lihttpto::Url& url, const Vec<String>& transient)
    -> String {
    if (transient.is_empty() || url.query().is_none()) return String::make(url.as_ref());
    auto query = lihttpto::QueryParams::parse_form_lossy(*url.query());
    for (const auto& name : transient) query.remove(name.as_str());
    auto result = String::make(*url.scheme());
    result.push_str("://"_str);
    result.push_str(*url.authority());
    result.push_str(url.path());
    if (! query.is_empty()) {
        result.push_ascii('?');
        result.push_str(query.encode_form().as_str());
    }
    if (url.fragment().is_some()) {
        result.push_ascii('#');
        result.push_str(*url.fragment());
    }
    return result;
}

auto PreparedRequest::make(ref<str> method, ref<str> url, lihttpto::Headers headers, bool has_body,
                           Option<Duration> timeout) -> ProviderResult<PreparedRequest> {
    if (method != "GET"_str || has_body)
        return Err(ProviderError::internal("resource request must be a bodyless GET"_str));
    auto parsed = rstd_try(lihttpto::Url::parse_http(url), [](auto error) {
        return ProviderError::external(rstd::move(error)).with_context("invalid resource URL"_str);
    });
    clear_conditions(headers);
    rstd_try(headers.set("accept-encoding"_str, "identity"_str), [](auto error) {
        return ProviderError::external(rstd::move(error))
            .with_context("invalid resource headers"_str);
    });
    auto cache_url = String::make(parsed.as_ref());
    auto delivery  = headers.clone();
    return Ok(PreparedRequest { rstd::move(parsed),
                                rstd::move(cache_url),
                                rstd::move(headers),
                                rstd::move(delivery),
                                timeout,
                                CacheMode::ByteRange,
                                {} });
}

auto PreparedRequest::with_transient_query(
    Vec<String> names) && -> ProviderResult<PreparedRequest> {
    if (names.len() > usize(32))
        return Err(ProviderError::internal("invalid transient query fields"_str));
    for (const auto& name : names)
        if (name.is_empty() || name.len() > usize(128))
            return Err(ProviderError::internal("invalid transient query fields"_str));
    transient_query_ = rstd::move(names);
    cache_url_       = normalized_url(url_, transient_query_);
    return Ok(rstd::move(*this));
}

auto PreparedRequest::request_parts() const -> lihttpto::RequestParts {
    static const auto method = lihttpto::Method::parse("GET"_str).unwrap();
    return { url_.clone(), method.clone(), delivery_headers_.clone() };
}

auto PreparedRequest::cache_url() const -> ref<str> { return cache_url_.as_str(); }

auto PreparedRequest::cache_url(ref<str> effective) const -> ProviderResult<String> {
    if (effective == url()) return Ok(cache_url_.clone());
    auto target = rstd_try(lihttpto::Url::parse_http(effective), [](auto error) {
        return ProviderError::external(rstd::move(error)).with_context("invalid resource URL"_str);
    });
    if (! url_.same_http_origin(target) || url_.path() != target.path())
        return Ok(String::make(target.as_ref()));
    return Ok(normalized_url(target, transient_query_));
}

auto PreparedRequest::apply_headers(const lihttpto::Headers& downstream) -> ProviderResult<empty> {
    auto                      delivery = delivery_headers_.clone();
    Vec<lihttpto::HeaderName> seen;
    for (const auto& entry : downstream) {
        auto name = entry.name.as_str();
        if (! resource_header_allowed(entry.name)) continue;
        bool duplicate = false;
        for (const auto& previous : seen)
            if (previous.matches(name)) duplicate = true;
        if (duplicate) continue;
        seen.push(entry.name.clone());
        delivery.remove(name);
        delivery.push(entry.clone());
    }
    headers_ = delivery.clone();
    clear_conditions(headers_);
    delivery_headers_ = rstd::move(delivery);
    return Ok(empty {});
}

auto PreparedRequest::replace_delivery_headers(const lihttpto::Headers& downstream)
    -> ProviderResult<empty> {
    auto request = clone();
    clear_conditions(request.delivery_headers_);
    rstd_try(request.apply_headers(downstream));
    *this = rstd::move(request);
    return Ok(empty {});
}

auto PreparedRequest::extend_delivery_headers(const lihttpto::Headers& headers) -> void {
    clear_conditions(delivery_headers_);
    for (const auto& field : headers) delivery_headers_.remove(field.name.as_str());
    for (const auto& field : headers) delivery_headers_.push(field.clone());
}

auto PreparedRequest::clone() const -> PreparedRequest {
    Vec<String> transient;
    for (const auto& field : transient_query_) transient.push(field.clone());
    return PreparedRequest {
        url_.clone(), cache_url_.clone(),   headers_.clone(), delivery_headers_.clone(), timeout_,
        mode_,        rstd::move(transient)
    };
}
} // namespace qcm::backend::resource_cache
