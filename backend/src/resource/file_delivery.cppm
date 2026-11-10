module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:resource.file_delivery;
export import :resource.model;
export import :resource.response;
export import rstd;
export import lihttpto;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend::resource_cache
{
class ResourceDelivery {
    RSTD_ENUM(ResourceDelivery, (Body, (Response response;)),
              (LocalFile, (qcm::backend::resource_cache::LocalFile file;)))
public:
    auto into_response() && -> Result<Response, Error>;
};

} // namespace qcm::backend::resource_cache

namespace qcm::backend::resource_cache
{
auto ResourceDelivery::into_response() && -> Result<Response, Error> {
    RSTD_MATCH(rstd::move(*this)) {
        RSTD_CASE(Body, response) { return Ok(rstd::move(response)); }
        RSTD_CASE(LocalFile, file) {
            auto path  = file.path.as_path();
            auto bytes = path.as_os_str().as_encoded_bytes();
            if (! path.is_absolute() || bytes.is_empty() || bytes[usize()] != u8('/'))
                return Err(Error::state("invalid local file URL"_str));
            auto           location = String::make("file://"_str);
            constexpr char hex[]    = "0123456789ABCDEF";
            for (auto byte : bytes) {
                auto value = byte.to_primitive();
                if ((value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
                    (value >= '0' && value <= '9') || value == '/' || value == '-' ||
                    value == '.' || value == '_' || value == '~' || value == ':')
                    location.push_ascii(static_cast<char>(value));
                else {
                    location.push_ascii('%');
                    location.push_ascii(hex[value >> 4]);
                    location.push_ascii(hex[value & 15]);
                }
            }
            Response response { i32(302), {}, {}, None() };
            auto&    headers = response.headers;
            auto     add     = [&](ref<str> name, ref<str> value) -> Result<empty, Error> {
                auto result = headers.add(name, value);
                if (result.is_err())
                    return Err(Error::state("invalid local file delivery header"_str,
                                            rstd::move(result).unwrap_err()));
                return Ok(empty {});
            };
            rstd_try(add("location"_str, location.as_str()));
            rstd_try(add("cache-control"_str, "no-store"_str));
            rstd_try(add("content-length"_str, "0"_str));
            rstd_try(add("qcm-file-lease"_str, file.token.as_str()));
            rstd_try(add("qcm-file-lease-seconds"_str, rstd::format("{}", file.lifetime).as_str()));
            rstd_try(add("qcm-file-generation"_str, file.generation.as_str()));
            rstd_try(add("qcm-file-renew"_str,
                         rstd::format("/resource-file-leases/{}/renew", file.token).as_str()));
            rstd_try(add("qcm-file-release"_str,
                         rstd::format("/resource-file-leases/{}", file.token).as_str()));
            return Ok(rstd::move(response));
        }
    }
}
} // namespace qcm::backend::resource_cache
