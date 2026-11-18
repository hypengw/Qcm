module;
#include <rstd/macro.hpp>
module qcm.backend;
import :provider.interface;
using rstd::async::coro;
using namespace rstd::prelude;

namespace qcm::backend
{
auto ProviderCommon::validate_settings(const SettingsValues& values)
    -> Result<SettingsValues, SettingsError> {
    return qcm::backend::validate_settings(settings_definition(), values);
}

auto ProviderSession::close_async() -> coro<empty> {
    invalidate();
    co_return empty {};
}

auto Provider::prepare_playback_resource_async(ref<str>, ref<str>, const lihttpto::Headers&)
    -> coro<ProviderResult<resource_cache::PreparedResource>> {
    co_return Err(ProviderError::NotImplemented());
}

auto Provider::prepare_resource_async(Resource, const lihttpto::Headers&)
    -> coro<ProviderResult<resource_cache::PreparedResource>> {
    co_return Err(ProviderError::NotImplemented());
}

auto Provider::for_you_items_async(ProviderContext) -> coro<ProviderResult<Vec<HomeEntry>>> {
    co_return Err(ProviderError::NotImplemented());
}

auto Provider::open_playback_async(PlaybackRequest) -> coro<ProviderResult<PreparedPlayback>> {
    co_return Ok(PreparedPlayback::byte_resource());
}

auto Provider::keep_playback_async(String) -> coro<ProviderResult<empty>> {
    co_return Ok(empty {});
}

auto Provider::close_playback_async(String) -> coro<ProviderResult<empty>> {
    co_return Ok(empty {});
}

auto Provider::comments_async(ProviderContext, provider_comment::CommentTarget,
                              provider_comment::CommentPageRequest)
    -> coro<ProviderResult<Option<provider_comment::CommentPage>>> {
    co_return Ok(None());
}

auto Provider::comment_replies_async(ProviderContext, provider_comment::CommentTarget, String,
                                     provider_comment::CommentPageRequest)
    -> coro<ProviderResult<Option<provider_comment::CommentPage>>> {
    co_return Ok(None());
}

auto Provider::set_comment_like_async(ProviderContext, provider_comment::CommentTarget, String,
                                      bool)
    -> coro<ProviderResult<provider_comment::CommentLikeResult>> {
    co_return Err(ProviderError::NotImplemented());
}

auto Provider::home_blocks_async(ProviderContext) -> coro<ProviderResult<Vec<HomeBlock>>> {
    co_return Err(ProviderError::NotImplemented());
}

auto Provider::home_block_items_async(ProviderContext, String, i32, i32)
    -> coro<ProviderResult<HomeBlockItems>> {
    co_return Err(ProviderError::NotImplemented());
}

auto Provider::qr_async() -> coro<ProviderResult<QrInfo>> {
    co_return Err(ProviderError::NotImplemented());
}

auto Provider::queue_definition_async(ProviderContext, String)
    -> coro<ProviderResult<RadioDefinition>> {
    co_return Ok(RadioDefinition {});
}

auto Provider::open_queue_async(ProviderContext context, String native_id, QueueOpenRequest request)
    -> coro<ProviderResult<RadioBatch>> {
    if (request.input.is_some()) co_return Err(ProviderError::NotImplemented());
    auto       state = rstd_co_try(legacy_queue_state(request.configuration));
    auto       batch = rstd_co_try(co_await legacy_queue_async(
        rstd::move(context), rstd::move(native_id), {}, request.configuration.clone()));
    RadioBatch result;
    result.songs         = rstd::move(batch.songs);
    result.state         = rstd::move(state);
    result.configuration = rstd::move(request.configuration);
    co_return Ok(rstd::move(result));
}

auto Provider::next_queue_async(ProviderContext context, String native_id, QueueNextRequest request)
    -> coro<ProviderResult<RadioBatch>> {
    auto configuration = rstd_co_try(legacy_queue_configuration(request.state.as_str()));
    auto batch = rstd_co_try(co_await legacy_queue_async(rstd::move(context),
                                                         rstd::move(native_id),
                                                         rstd::move(request.current_native_ids),
                                                         configuration.clone()));
    RadioBatch result;
    result.songs         = rstd::move(batch.songs);
    result.state         = rstd::move(request.state);
    result.configuration = rstd::move(configuration);
    co_return Ok(rstd::move(result));
}

auto Provider::legacy_queue_async(ProviderContext, String, Vec<String>, QueueConfiguration)
    -> coro<ProviderResult<RadioBatch>> {
    co_return Err(ProviderError::NotImplemented());
}

auto Provider::close_queue_async(ProviderContext, String, String) -> coro<ProviderResult<empty>> {
    co_return Ok(empty {});
}
} // namespace qcm::backend
