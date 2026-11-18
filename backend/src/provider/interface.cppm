export module qcm.backend:provider.interface;
export import :model.home;
export import :provider.dto;
export import :provider.common;
export import :provider.context;
export import :provider.playback;
export import :provider.resource;
export import :provider.comment;
export import :provider.settings;
export import :provider.generation;
export import :provider.queue;
export import :model.subtitle;
export import :model.image;
export import :network;
using alloc::rc::Rc;
using rstd::async::coro;
using namespace rstd::prelude;

export namespace qcm::backend
{
class ProviderCommon {
public:
    virtual ~ProviderCommon()                                            = default;
    virtual auto status_snapshot() const -> ProviderCommonSnapshot       = 0;
    virtual auto base_url() const -> String                              = 0;
    virtual auto auth_method() const -> Option<AuthMethod>               = 0;
    virtual auto settings() const -> SettingsSnapshot                    = 0;
    virtual auto set_settings(SettingsSnapshot) -> ProviderResult<empty> = 0;
    virtual auto set_name(ref<str>) -> ProviderResult<empty>             = 0;
    virtual auto set_identity(i64, ref<str>) -> ProviderResult<empty>    = 0;
    virtual auto set_auth_info(AuthInput) -> ProviderResult<empty>       = 0;
    virtual auto settings_definition() const -> slice<SettingField> { return {}; }
    virtual auto validate_settings(const SettingsValues& values)
        -> Result<SettingsValues, SettingsError>;
};

class ProviderSession {
public:
    virtual ~ProviderSession()                                  = default;
    virtual auto save_cookie() -> ProviderResult<String>        = 0;
    virtual auto load_cookie(ref<str>) -> ProviderResult<empty> = 0;
    virtual auto set_session_store(sqlito::ExecutorHandle, i64, String) -> void {}
    virtual auto matches_session(i64, ref<str>) const -> bool { return false; }
    virtual auto session_key() const -> Option<String> { return None(); }
    virtual auto invalidate() -> void = 0;
    virtual auto cancel_streams() -> void {}
    virtual auto close_async() -> coro<empty>;
    virtual auto resource_cookie_session() const -> CookieSession    = 0;
    virtual auto session_epoch() const -> Option<u64>                = 0;
    virtual auto lifecycle_epoch() const -> u64                      = 0;
    virtual auto generation_check() const -> ProviderGenerationCheck = 0;
};

class Provider : public ProviderCommon, public ProviderSession {
public:
    ~Provider() override                                                    = default;
    virtual auto auth_async(AuthInput) -> coro<ProviderResult<AuthOutcome>> = 0;
    virtual auto load_session(ref<str>) -> ProviderResult<empty>            = 0;
    virtual auto save_session() -> ProviderResult<String>                   = 0;
    virtual auto resource_scope() -> ProviderResult<Option<String>> { return Ok(None()); }
    virtual auto sync_async(ProviderContext, Option<Item>) -> coro<ProviderResult<u32>> = 0;
    virtual auto prepare_playback_resource_async(ref<str>, ref<str>, const lihttpto::Headers&)
        -> coro<ProviderResult<resource_cache::PreparedResource>>;
    virtual auto prepare_resource_async(Resource, const lihttpto::Headers&)
        -> coro<ProviderResult<resource_cache::PreparedResource>>;
    virtual auto for_you_items_async(ProviderContext) -> coro<ProviderResult<Vec<HomeEntry>>>;
    virtual auto open_playback_async(PlaybackRequest) -> coro<ProviderResult<PreparedPlayback>>;
    virtual auto keep_playback_async(String) -> coro<ProviderResult<empty>>;
    virtual auto close_playback_async(String) -> coro<ProviderResult<empty>>;
    virtual auto comments_async(ProviderContext, provider_comment::CommentTarget,
                                provider_comment::CommentPageRequest)
        -> coro<ProviderResult<Option<provider_comment::CommentPage>>>;
    virtual auto comment_replies_async(ProviderContext, provider_comment::CommentTarget, String,
                                       provider_comment::CommentPageRequest)
        -> coro<ProviderResult<Option<provider_comment::CommentPage>>>;
    virtual auto set_comment_like_async(ProviderContext, provider_comment::CommentTarget, String,
                                        bool)
        -> coro<ProviderResult<provider_comment::CommentLikeResult>>;
    virtual auto home_blocks_async(ProviderContext) -> coro<ProviderResult<Vec<HomeBlock>>>;
    virtual auto home_block_items_async(ProviderContext, String, i32, i32)
        -> coro<ProviderResult<HomeBlockItems>>;
    virtual auto qr_async() -> coro<ProviderResult<QrInfo>>;
    virtual auto share_url(ref<str>, ItemType) -> ProviderResult<Option<lihttpto::Url>> {
        return Ok(None());
    }
    virtual auto queue_definition_async(ProviderContext, String)
        -> coro<ProviderResult<RadioDefinition>>;
    virtual auto open_queue_async(ProviderContext context, String native_id,
                                  QueueOpenRequest request) -> coro<ProviderResult<RadioBatch>>;
    virtual auto next_queue_async(ProviderContext context, String native_id,
                                  QueueNextRequest request) -> coro<ProviderResult<RadioBatch>>;
    virtual auto legacy_queue_async(ProviderContext, String, Vec<String>, QueueConfiguration)
        -> coro<ProviderResult<RadioBatch>>;
    virtual auto close_queue_async(ProviderContext, String, String) -> coro<ProviderResult<empty>>;
    virtual auto subtitle_async(String) -> coro<ProviderResult<SubtitleData>> = 0;
    virtual auto favorite_async(ProviderContext, String, ItemType, bool)
        -> coro<ProviderResult<empty>> = 0;
};
using ProviderHandle       = Rc<Provider>;
using ProviderCancelHandle = Rc<bool>;
} // namespace qcm::backend
