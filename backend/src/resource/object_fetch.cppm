module;
#include <rstd/enum.hpp>
export module qcm.backend:resource.object_fetch;
export import :resource.service;
export import :provider.runtime;
using rstd::async::coro;
using rstd::sync::Arc;
using namespace rstd::prelude;
using namespace rstd::literals;
using ResourceError    = qcm::backend::resource_cache::Error;
using ResourceResponse = qcm::backend::resource_cache::Response;
using ResourceDelivery = qcm::backend::resource_cache::ResourceDelivery;

export namespace qcm::backend
{
struct PlaybackObjectScope {
    String           scope;
    PlaybackLifetime lifetime;
};

class ResourceObjectFetch {
public:
    auto respond(resource_cache::ResourceCache* cache, ProviderResourcePlan plan,
                 lihttpto::Headers headers, Option<PlaybackObjectScope> playback = None())
        -> coro<Result<ResourceDelivery, ResourceError>> {
        auto lifetime =
            playback.is_some() ? Some(playback->lifetime.clone()) : None<PlaybackLifetime>();
        if (lifetime && ! (*lifetime)->active()) co_return Err(ResourceError::invalidated());
        // Image HTTP caching belongs to the frontend network stack.
        if (plan.kind == resource_cache::ResourceKind::Image || cache == nullptr) {
            plan.preparation = rstd::move(plan.preparation).with_lifetime(rstd::move(lifetime));
            auto response =
                rstd_co_try(co_await rstd::move(plan).execute_async(true), ResourceError::provider);
            co_return Ok(ResourceDelivery::Body(rstd::move(response)));
        }
        auto scope    = playback.is_some()
                            ? rstd::move(playback->scope)
                            : rstd_co_try(plan.resource_scope(), ResourceError::provider);
        auto kind     = plan.kind;
        auto provider = plan.provider_id;
        auto prepared = Arc<resource_cache::PreparedResource>::make(
            rstd::move(plan.preparation).with_lifetime(rstd::move(lifetime)));
        auto validity = prepared->guard();
        auto delivery = rstd_co_try(co_await cache->delivery(resource_cache::ResourceRequest {
            provider, kind, rstd::move(scope), rstd::move(prepared), rstd::move(headers) }));
        RSTD_MATCH(delivery) {
            RSTD_CASE(Body, response) {
                co_return Ok(ResourceDelivery::Body(validity->guard(rstd::move(response))));
            }
            RSTD_CASE(LocalFile, file) {
                co_return Ok(ResourceDelivery::LocalFile(rstd::move(file)));
            }
        }
    }
};
} // namespace qcm::backend
