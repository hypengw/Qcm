module;
#include <rstd/enum.hpp>
export module qcm.backend:api.provider;
import :protocol.message;
import :provider.registry;
import :provider.runtime;
import :provider.item;
import :db.database;
import :db.query.subtitle;
import :db.sync.run;
import :error;
import sqlito;
import rstd;
using alloc::rc::Rc;
using rstd::collections::BTreeMap;
using rstd::sync::Arc;
using namespace rstd::prelude;

export namespace qcm::backend
{
struct TmpProviderEntry {
    String                         key;
    String                         type_name;
    ProviderHandle                 instance;
    bool                           promoted {};
    Arc<rstd::async::Mutex<empty>> gate { Arc<rstd::async::Mutex<empty>>::make(empty {}) };
};
using TmpProviderHandle = Rc<TmpProviderEntry>;

class TmpProviderStore {
    BTreeMap<String, TmpProviderHandle> entries_;

public:
    auto find(ref<str> key) -> TmpProviderEntry*;
    auto acquire(ref<str> key) -> TmpProviderHandle;
    auto erase(ref<str> key) -> bool;
    auto push(TmpProviderEntry entry) -> void;
    auto len() const -> usize;
    auto close_async() -> rstd::async::coro<empty>;
};

enum class ProviderMutationKind
{
    Added,
    Updated,
    Replaced,
    Deleted
};
class ProviderLiveMutation {
    RSTD_ENUM(
        ProviderLiveMutation,
        (Add, (ProviderMetaInfo meta; TmpProviderHandle temporary; String name; String key;)),
        (Update, (ProviderHandle instance; String name; SettingsSnapshot settings;)),
        (Replace, (
            ProviderHandle previous;
            ProviderHandle candidate;
            TmpProviderHandle temporary;
            String name;
            String key;
            SettingsSnapshot settings;
        )),
        (Delete)
    )
};
struct ProviderCommittedMutation {
    ProviderMutationKind kind;
    i64                  provider_id;
    ProviderLiveMutation live;

    auto install(ProviderRuntime& runtime, sqlito::ExecutorHandle db) -> ProviderHandle;
    auto replaces_instance() const -> bool;
};
class ProviderMutationOutcome {
    RSTD_ENUM(
        ProviderMutationOutcome,
        (Committed, (ProviderCommittedMutation mutation;)),
        (SettingsRejected, (SettingsError error;)),
        (AuthenticationRejected, (AuthOutcome outcome;))
    )
};

class ProviderMutationWrite {
    ProviderCancelHandle cancellable_;

public:
    explicit ProviderMutationWrite(ProviderCancelHandle cancellable = {})
        : cancellable_(rstd::move(cancellable)) {}
    template<class F>
    auto run(sqlito::ExecutorHandle db, F function)
        -> rstd::async::coro<decltype(function(rstd::declval<sqlito::Connection&>()))> {
        // The worker may commit before its result wakes this task. Seal cancellation before submit.
        if (cancellable_) *cancellable_.get() = false;
        co_return co_await database_run(rstd::move(db), rstd::move(function));
    }
};

class ProviderAuthenticationUpdate {
    RSTD_ENUM(
        ProviderAuthenticationUpdate,
        (Unchanged),
        (Authenticate, (AuthInput input;)),
        (Conflict)
    )
};
struct ProviderUpdateRequest {
    i64                          provider_id;
    String                       name;
    String                       temporary;
    String                       settings_json;
    u64                          settings_revision;
    ProviderAuthenticationUpdate authentication;
};

struct SubtitlePlan {
    i64            song_id;
    SubtitleTarget target;
    ProviderHandle instance;
    u64            epoch;
};
struct FavoritePlan {
    ProviderItem   target;
    ProviderHandle instance;
    u64            epoch;
};

auto get_provider_metas(const ProviderRegistry& registry, i32 request_id)
    -> p_qcm::p_msg::M_QcmMessage;
auto json_escape(ref<str> value) -> String;
auto provider_ok(i32 request_id) -> p_qcm::p_msg::M_QcmMessage;
auto create_tmp_provider(TmpProviderStore& store, const ProviderRegistry& registry,
                         ProviderRuntime& runtime, i32 request_id,
                         const p_qcm::p_msg::M_CreateTmpProviderReq& entry)
    -> Result<p_qcm::p_msg::M_QcmMessage, ProcessError>;
auto delete_tmp_provider(TmpProviderStore& store, i32 request_id,
                         const p_qcm::p_msg::M_DeleteTmpProviderReq& entry)
    -> Result<p_qcm::p_msg::M_QcmMessage, ProcessError>;
auto auth_response(i32 request_id, AuthOutcome outcome) -> p_qcm::p_msg::M_QcmMessage;
auto auth_provider_async(TmpProviderStore& store, i32 request_id,
                         const p_qcm::p_msg::M_AuthProviderReq& entry)
    -> rstd::async::coro<Result<p_qcm::p_msg::M_QcmMessage, ProcessError>>;
auto add_provider_async(TmpProviderStore& store, const ProviderRegistry& registry,
                        sqlito::ExecutorHandle db, const p_qcm::p_msg::M_AddProviderReq& entry,
                        ProviderMutationWrite& write)
    -> rstd::async::coro<ProcessResult<ProviderMutationOutcome>>;
auto get_provider_settings(ProviderRuntime& runtime, i32 request_id,
                           const p_qcm::p_msg::M_GetProviderSettingsReq& entry)
    -> ProcessResult<p_qcm::p_msg::M_QcmMessage>;
auto prepare_subtitle(ProviderRuntime& runtime, sqlito::ExecutorHandle db,
                      const p_qcm::p_msg::M_GetSubtitleReq& entry)
    -> rstd::async::coro<Result<SubtitlePlan, ProcessError>>;
auto execute_subtitle(ProviderRuntime& runtime, sqlito::ExecutorHandle db, i32 request_id,
                      SubtitlePlan plan)
    -> rstd::async::coro<Result<p_qcm::p_msg::M_QcmMessage, ProcessError>>;
auto prepare_favorite(ProviderRuntime& runtime, sqlito::ExecutorHandle db,
                      const p_qcm::p_msg::M_SetFavoriteReq& entry)
    -> rstd::async::coro<Result<FavoritePlan, ProcessError>>;
auto execute_favorite(ProviderRuntime& runtime, SyncCoordinator& coordinator,
                      sqlito::ExecutorHandle db, i32 request_id,
                      const p_qcm::p_msg::M_SetFavoriteReq& entry, FavoritePlan plan)
    -> rstd::async::coro<Result<p_qcm::p_msg::M_QcmMessage, ProcessError>>;
auto update_auth_response(i32 request_id, AuthOutcome outcome) -> p_qcm::p_msg::M_QcmMessage;
auto update_provider_async(TmpProviderStore& store, ProviderRuntime& runtime,
                           SyncCoordinator& coordinator, const ProviderRegistry& registry,
                           sqlito::ExecutorHandle db, ProviderUpdateRequest entry,
                           ProviderMutationWrite& write)
    -> rstd::async::coro<ProcessResult<ProviderMutationOutcome>>;
auto provider_update_request(const p_qcm::p_msg::M_UpdateProviderReq& entry)
    -> ProcessResult<ProviderUpdateRequest>;
auto replace_provider_async(TmpProviderStore& store, ProviderRuntime& runtime,
                            SyncCoordinator& coordinator, const ProviderRegistry& registry,
                            sqlito::ExecutorHandle                    db,
                            const p_qcm::p_msg::M_ReplaceProviderReq& entry,
                            ProviderMutationWrite&                    write)
    -> rstd::async::coro<ProcessResult<ProviderMutationOutcome>>;
auto delete_provider_async(sqlito::ExecutorHandle db, SyncCoordinator& coordinator,
                           const p_qcm::p_msg::M_DeleteProviderReq& entry,
                           ProviderMutationWrite&                   write)
    -> rstd::async::coro<ProcessResult<ProviderMutationOutcome>>;
auto provider_mutation_response(i32 request_id, ProviderMutationOutcome outcome)
    -> p_qcm::p_msg::M_QcmMessage;
auto load_radio_queues_async(ProviderRuntime& runtime, sqlito::ExecutorHandle db,
                             SyncCoordinator& coordinator, Vec<i64> providers,
                             ProviderRequestOwnerHandle owner = {})
    -> rstd::async::coro<Result<Vec<Box<p_qcm::p_msg::p_model::M_RadioQueue>>, ProcessError>>;
auto get_radio_queues_async(ProviderRuntime& runtime, sqlito::ExecutorHandle db,
                            SyncCoordinator& coordinator, i32 request_id,
                            const p_qcm::p_msg::M_GetRadioQueuesReq& entry,
                            ProviderRequestOwnerHandle               owner = {})
    -> rstd::async::coro<Result<p_qcm::p_msg::M_QcmMessage, ProcessError>>;
auto qr_auth_url_async(TmpProviderStore& store, i32 request_id,
                       const p_qcm::p_msg::M_QrAuthUrlReq& entry)
    -> rstd::async::coro<Result<p_qcm::p_msg::M_QcmMessage, ProcessError>>;
auto get_share_link_async(ProviderRuntime& runtime, sqlito::ExecutorHandle db, i32 request_id,
                          const p_qcm::p_msg::M_GetShareLinkReq& entry)
    -> rstd::async::coro<Result<p_qcm::p_msg::M_QcmMessage, ProcessError>>;
} // namespace qcm::backend
