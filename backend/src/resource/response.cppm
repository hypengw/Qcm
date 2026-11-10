export module qcm.backend:resource.response;
export import :resource.body;
export import lihttpto;
using lihttpto::Headers;
using rstd::async::coro;
using rstd::bytes::Bytes;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend::resource_cache
{
struct Response {
    i32            status {};
    Headers        headers;
    ResourceBody   body;
    Option<String> effective_url;

    auto location() const -> Option<String> {
        auto field = headers.get("location"_str);
        if (field.is_none()) return None();
        auto text = (*field)->to_str();
        if (text.is_err()) return None();
        return Some(String::make(*text));
    }
    auto content_length() const -> Option<u64> {
        auto length = lihttpto::content_length(headers);
        if (length.is_err()) return None();
        return rstd::move(length).unwrap();
    }
    auto next() -> coro<Result<Option<Bytes>, Error>> { return body.next(); }
    auto cancel() -> void { body.cancel(); }
    auto close() -> coro<empty> { return body.close(); }
    auto set_body(Bytes bytes) -> Result<empty, Error> {
        auto updated = headers.clone();
        updated.remove("content-length"_str);
        updated.remove("transfer-encoding"_str);
        auto length = rstd::format("{}", bytes.len());
        auto result = updated.set("content-length"_str, length.as_str());
        if (result.is_err())
            return Err(
                Error::state("invalid resource body length"_str, rstd::move(result).unwrap_err()));
        body    = ResourceBody::from_bytes(rstd::move(bytes));
        headers = rstd::move(updated);
        return Ok(empty {});
    }
};
} // namespace qcm::backend::resource_cache
