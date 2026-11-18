export module qcm.backend:resource.response;
export import :resource.body;
export import lihttpto;
using lihttpto::Headers;
using rstd::async::coro;
using rstd::bytes::Bytes;
using namespace rstd::prelude;

export namespace qcm::backend::resource_cache
{
struct Response {
    i32            status {};
    Headers        headers;
    ResourceBody   body;
    Option<String> effective_url;

    auto location() const -> Option<String>;
    auto content_length() const -> Option<u64>;
    auto next() -> coro<Result<Option<Bytes>, Error>> { return body.next(); }
    auto cancel() -> void { body.cancel(); }
    auto close() -> coro<empty> { return body.close(); }
    auto set_body(Bytes bytes) -> Result<empty, Error>;
};
} // namespace qcm::backend::resource_cache
