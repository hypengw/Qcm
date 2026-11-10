module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:api.comments;
export import :provider.runtime;
export import :error;
import :protocol.message;
import :api.provider;
import :provider.item;
import :db.sync.run;
import licrypto;
import rstd.json;
export import rstd;
using rstd::json::Number;
using rstd::json::Value;
using namespace rstd::prelude;
using namespace rstd::literals;
using QcmMessage        = p_qcm::p_msg::M_QcmMessage;
using Comment           = p_qcm::p_msg::p_model::M_Comment;
using CommentAuthor     = p_qcm::p_msg::p_model::M_CommentAuthor;
using CommentPage       = p_qcm::p_msg::p_model::M_CommentPage;
using CommentsResult    = p_qcm::p_msg::p_model::M_CommentsResult;
namespace comment_model = qcm::backend::provider_comment;

namespace qcm::backend::comment_detail
{
struct CommentToken {
    String         scope;
    i64            id {};
    i32            kind {};
    i32            order {};
    Option<String> parent;
    u32            limit {};
    String         state;
};

constexpr auto rstd_serde_definition(CommentToken*) {
    using namespace rstd::serde;
    return RecordDefinition<CommentToken,
                            "CommentToken",
                            Direction::Both,
                            UnknownFieldPolicy::Ignore,
                            Member<&CommentToken::scope, "scope">,
                            Member<&CommentToken::id, "id">,
                            Member<&CommentToken::kind, "kind">,
                            Member<&CommentToken::order, "order">,
                            Member<&CommentToken::parent, "parent", FieldPolicy::Defaulted>,
                            Member<&CommentToken::limit, "limit">,
                            Member<&CommentToken::state, "state">> {};
}

auto decode_comment_token(ref<str> text) -> ProcessResult<CommentToken> {
    if (text.len() > usize(16384)) return Err(ProcessError::MissingFields("comment token"_Str));
    return rstd::json::decode<CommentToken>(text).map_err([](auto) {
        return ProcessError::MissingFields("comment token"_Str);
    });
}

auto comment_state(const p_qcm::p_msg::M_GetCommentsReq& entry, ref<str> scope,
                   Option<ref<str>> parent) -> ProcessResult<Option<String>> {
    if (entry.f_token.is_none()) return Ok(None());
    auto token = rstd_try(decode_comment_token(entry.f_token->as_str()));
    if (token.scope.as_str() != scope || token.id != entry.f_id ||
        token.kind != entry.f_item_type || token.order != entry.f_order ||
        token.parent.is_some() != parent.is_some() ||
        (parent.is_some() && token.parent->as_str() != *parent) || token.limit != entry.f_limit)
        return Err(ProcessError::MissingFields("expired or mismatched comment token"_Str));
    return Ok(Some(rstd::move(token.state)));
}

auto comment_page_order(const p_qcm::p_msg::M_GetCommentsReq& entry, Option<ref<str>> parent)
    -> ProcessResult<comment_model::CommentOrder> {
    if (entry.f_limit < u32(1) || entry.f_limit > u32(50) ||
        (parent.is_some() && (parent->is_empty() || parent->len() > usize(256))))
        return Err(ProcessError::MissingFields("comment page parameters"_Str));
    if (entry.f_order == i32()) return Ok(comment_model::CommentOrder::Latest);
    if (entry.f_order == i32(1)) return Ok(comment_model::CommentOrder::Hot);
    return Err(ProcessError::MissingFields("comment order"_Str));
}

auto comment_order_value(comment_model::CommentOrder order) -> i32 {
    return order == comment_model::CommentOrder::Hot ? i32(1) : i32();
}
} // namespace qcm::backend::comment_detail

namespace rstd
{
template<>
struct Impl<convert::From<comment_model::CommentAuthor>, Box<CommentAuthor>> {
    static auto from(comment_model::CommentAuthor source) -> Box<CommentAuthor> {
        auto author          = Box<CommentAuthor>::make();
        author->f_id         = rstd::move(source.id);
        author->f_name       = rstd::move(source.name);
        author->f_avatar_url = rstd::move(source.avatar_url);
        return author;
    }
};

template<>
struct Impl<convert::From<comment_model::CommentQuote>,
            Box<p_qcm::p_msg::p_model::M_CommentQuote>> {
    static auto from(comment_model::CommentQuote source)
        -> Box<p_qcm::p_msg::p_model::M_CommentQuote> {
        auto quote       = Box<p_qcm::p_msg::p_model::M_CommentQuote>::make();
        quote->f_id      = rstd::move(source.id);
        quote->f_author  = Some(rstd::into<Box<CommentAuthor>>(rstd::move(source.author)));
        quote->f_content = rstd::move(source.content);
        quote->f_deleted = source.deleted;
        return quote;
    }
};

template<>
struct Impl<convert::From<comment_model::Comment>, Box<Comment>> {
    static auto from(comment_model::Comment source) -> Box<Comment> {
        auto comment           = Box<Comment>::make();
        comment->f_id          = rstd::move(source.id);
        comment->f_author      = Some(rstd::into<Box<CommentAuthor>>(rstd::move(source.author)));
        comment->f_content     = rstd::move(source.content);
        comment->f_time_ms     = rstd::move(source.time_ms);
        comment->f_liked       = rstd::move(source.liked);
        comment->f_like_count  = rstd::move(source.like_count);
        comment->f_reply_count = rstd::move(source.reply_count);
        comment->f_deleted     = source.deleted;
        comment->f_quote = rstd::move(source.quote).map([](comment_model::CommentQuote quote) {
            return rstd::into<Box<p_qcm::p_msg::p_model::M_CommentQuote>>(rstd::move(quote));
        });
        for (auto& preview : source.previews)
            comment->f_previews.push(rstd::into<Box<Comment>>(rstd::move(preview)));
        return comment;
    }
};

template<>
struct Impl<convert::From<comment_model::CommentPage>, Box<CommentPage>> {
    static auto from(comment_model::CommentPage source) -> Box<CommentPage> {
        using qcm::backend::comment_detail::comment_order_value;
        auto page = Box<CommentPage>::make();
        for (auto& comment : source.comments)
            page->f_comments.push(rstd::into<Box<Comment>>(rstd::move(comment)));
        page->f_order = comment_order_value(source.order);
        for (auto order : source.orders) page->f_orders.push(comment_order_value(order));
        page->f_total     = rstd::move(source.total);
        page->f_has_more  = source.has_more;
        page->f_can_reply = source.can_reply;
        page->f_can_like  = source.can_like;
        return page;
    }
};
} // namespace rstd

namespace qcm::backend::comment_detail
{
auto comment_page_message(comment_model::CommentPage source, ref<str> scope,
                          const p_qcm::p_msg::M_GetCommentsReq& entry, Option<ref<str>> parent)
    -> ProcessResult<Box<CommentPage>> {
    Option<String> next_token;
    if (source.next_state.is_some()) {
        CommentToken token { String::make(scope),
                             entry.f_id,
                             entry.f_item_type,
                             entry.f_order,
                             parent.map([](ref<str> value) {
                                 return String::make(value);
                             }),
                             entry.f_limit,
                             rstd::move(source.next_state).unwrap() };
        next_token =
            Some(rstd_try(rstd::json::encode(token), ProcessError::internal(rstd::move(_e))));
    }
    auto page          = rstd::into<Box<CommentPage>>(rstd::move(source));
    page->f_next_token = rstd::move(next_token);
    return Ok(rstd::move(page));
}
} // namespace qcm::backend::comment_detail

export namespace qcm::backend
{
struct CommentTarget {
    i64            id {};
    i64            provider_id {};
    String         native_id;
    ItemType       item_type { ItemType::Unspecified };
    ProviderHandle instance;
    u64            epoch {};
    String         scope;
};
auto comment_error(ProviderError error) -> ProcessError {
    RSTD_MATCH(error.root()) {
        RSTD_CASE(NotImplemented) { return ProcessError::NotImplemented(); }
        RSTD_CASE_DEFAULT { return ProcessError(rstd::move(error)); }
    }
}
auto comment_scope(ProviderRuntime& runtime, const CommentTarget& target)
    -> Result<String, ProcessError> {
    auto snapshot = runtime.snapshot(target.provider_id);
    if (snapshot.is_none() || ! ProviderHandle::ptr_eq(snapshot->instance, target.instance))
        return Err(ProcessError::NotAuth());
    static const auto instance = new_instance_id();
    Vec<Value>        fields;
    fields.push(Value::String(instance.clone()));
    fields.push(Value::Number(Number::from_u64(snapshot->stamp)));
    fields.push(Value::Number(Number::from_i64(target.provider_id)));
    fields.push(Value::Number(Number::from_i64(target.id)));
    fields.push(Value::String(target.native_id.clone()));
    fields.push(Value::Number(Number::from_i64(i64(static_cast<int>(target.item_type)))));
    auto identity = rstd::json::to_string(Value::Array(rstd::move(fields)));
    auto digest   = licrypto::sha256(identity.as_str().as_bytes());
    if (digest.is_err()) return Err(ProcessError::internal(rstd::move(digest).unwrap_err()));
    return Ok(licrypto::base64_encode(digest->as_slice()));
}
auto check_comment_scope(ProviderRuntime& runtime, const CommentTarget& target, ref<str> expected)
    -> Result<empty, ProcessError> {
    if (expected.is_empty() || rstd_try(comment_scope(runtime, target)).as_str() != expected)
        return Err(ProcessError::NotAuth());
    return Ok(empty {});
}
auto resolve_comment_target(sqlito::ExecutorHandle db, ProviderRuntime& runtime, i64 item_id,
                            i64 item_type)
    -> rstd::async::coro<Result<Option<CommentTarget>, ProcessError>> {
    auto resolved =
        rstd_co_try(co_await resolve_provider_item(db.clone(), runtime, item_id, item_type));
    if (resolved.is_none()) co_return Ok(None());
    auto snapshot = runtime.snapshot(resolved->provider_id);
    if (snapshot.is_none()) co_return Err(ProcessError::NotAuth());
    CommentTarget target;
    target.id          = resolved->id;
    target.provider_id = resolved->provider_id;
    target.native_id   = rstd::move(resolved->native_id);
    target.item_type   = resolved->type;
    target.instance    = rstd::move(snapshot->instance);
    target.epoch       = snapshot->stamp;
    target.scope       = rstd_co_try(comment_scope(runtime, target));
    co_return Ok(Some(rstd::move(target)));
}
template<class T, class Execute>
auto call_comment_provider(ProviderRuntime& runtime, sqlito::ExecutorHandle db,
                           SyncCoordinator& coordinator, const CommentTarget& target,
                           ProviderRequestOwnerHandle owner, Execute execute)
    -> rstd::async::coro<ProviderResult<T>> {
    auto snapshot = runtime.snapshot(target.provider_id);
    if (snapshot.is_none() || snapshot->stamp != target.epoch ||
        ! ProviderHandle::ptr_eq(snapshot->instance, target.instance) || ! snapshot->is_current())
        co_return Err(ProviderError::NotAuth());
    auto instance = snapshot->instance.clone();
    auto context  = snapshot->context(db.clone(), coordinator.clone());
    if (owner) {
        owner.get()->provider_id = target.provider_id;
        owner.get()->instance    = instance;
    }
    auto result = co_await execute(*instance.get(), rstd::move(context));
    if (owner) owner.get()->instance.reset();
    if (! snapshot->is_current()) co_return Err(ProviderError::NotAuth());
    co_return rstd::move(result);
}
auto comments_result(bool supported, ref<str> scope, i64 provider_id, u64 epoch,
                     Option<Box<CommentPage>> page) -> Box<CommentsResult> {
    auto result         = Box<CommentsResult>::make();
    result->f_supported = supported;
    result->f_scope     = String::make(scope);
    if (supported) {
        auto id               = Box<p_qcm::p_msg::p_id::M_ProviderId>::make();
        id->f_value           = provider_id;
        result->f_provider_id = Some(rstd::move(id));
        result->f_epoch       = epoch;
        result->f_page        = rstd::move(page);
    }
    return result;
}
auto unsupported_comment_replies(i32 request_id) -> QcmMessage {
    auto rsp      = Box<p_qcm::p_msg::M_GetCommentRepliesRsp>::make();
    rsp->f_result = Some(comments_result(false, ""_str, i64(), u64(), None()));
    return qcm_message(request_id, rstd::move(rsp));
}
auto comments_async(ProviderRuntime& runtime, sqlito::ExecutorHandle db,
                    SyncCoordinator& coordinator, i32 request_id,
                    const p_qcm::p_msg::M_GetCommentsReq& entry,
                    ProviderRequestOwnerHandle            owner = {})
    -> rstd::async::coro<Result<QcmMessage, ProcessError>> {
    auto order    = rstd_co_try(comment_detail::comment_page_order(entry, None()));
    auto resolved = rstd_co_try(co_await resolve_comment_target(
        db.clone(), runtime, entry.f_id, i64(entry.f_item_type.to_primitive())));
    if (resolved.is_none()) {
        auto rsp      = Box<p_qcm::p_msg::M_GetCommentsRsp>::make();
        rsp->f_result = Some(comments_result(false, ""_str, i64(), u64(), None()));
        co_return Ok(qcm_message(request_id, rstd::move(rsp)));
    }
    auto& target = *resolved;
    auto  state  = rstd_co_try(comment_detail::comment_state(entry, target.scope.as_str(), None()));
    auto  loaded = co_await call_comment_provider<Option<comment_model::CommentPage>>(
        runtime,
        db.clone(),
        coordinator,
        target,
        owner,
        [native_id = target.native_id.clone(),
         item_type = target.item_type,
         order,
         limit = entry.f_limit,
         state = rstd::move(state)](Provider& instance, ProviderContext context) mutable {
            return instance.comments_async(
                rstd::move(context),
                comment_model::CommentTarget { rstd::move(native_id), item_type },
                comment_model::CommentPageRequest { order, rstd::move(state), limit });
        });
    auto page_content = rstd_co_try(rstd::move(loaded), comment_error);
    rstd_co_try(check_comment_scope(runtime, target, target.scope.as_str()));
    if (page_content.is_none()) {
        auto rsp      = Box<p_qcm::p_msg::M_GetCommentsRsp>::make();
        rsp->f_result = Some(comments_result(false, ""_str, i64(), u64(), None()));
        co_return Ok(qcm_message(request_id, rstd::move(rsp)));
    }
    auto page     = rstd_co_try(comment_detail::comment_page_message(
        rstd::move(*page_content), target.scope.as_str(), entry, None()));
    auto rsp      = Box<p_qcm::p_msg::M_GetCommentsRsp>::make();
    rsp->f_result = Some(comments_result(
        true, target.scope.as_str(), target.provider_id, target.epoch, Some(rstd::move(page))));
    co_return Ok(qcm_message(request_id, rstd::move(rsp)));
}
auto comment_replies_async(ProviderRuntime& runtime, sqlito::ExecutorHandle db,
                           SyncCoordinator& coordinator, i32 request_id,
                           const p_qcm::p_msg::M_GetCommentRepliesReq& entry,
                           ProviderRequestOwnerHandle                  owner = {})
    -> rstd::async::coro<Result<QcmMessage, ProcessError>> {
    if (entry.f_query.is_none())
        co_return Err(ProcessError::MissingFields(String::make("comment query"_str)));
    const auto& query    = **entry.f_query;
    auto        parent   = Some(entry.f_parent_id.as_str());
    auto        order    = rstd_co_try(comment_detail::comment_page_order(query, parent));
    auto        resolved = rstd_co_try(co_await resolve_comment_target(
        db.clone(), runtime, query.f_id, i64(query.f_item_type.to_primitive())));
    if (resolved.is_none()) co_return Ok(unsupported_comment_replies(request_id));
    auto& target = *resolved;
    rstd_co_try(check_comment_scope(runtime, target, entry.f_scope.as_str()));
    auto state  = rstd_co_try(comment_detail::comment_state(query, target.scope.as_str(), parent));
    auto loaded = co_await call_comment_provider<Option<comment_model::CommentPage>>(
        runtime,
        db.clone(),
        coordinator,
        target,
        owner,
        [native_id = target.native_id.clone(),
         item_type = target.item_type,
         parent    = entry.f_parent_id.clone(),
         order,
         limit = query.f_limit,
         state = rstd::move(state)](Provider& instance, ProviderContext context) mutable {
            return instance.comment_replies_async(
                rstd::move(context),
                comment_model::CommentTarget { rstd::move(native_id), item_type },
                rstd::move(parent),
                comment_model::CommentPageRequest { order, rstd::move(state), limit });
        });
    auto page_content = rstd_co_try(rstd::move(loaded), comment_error);
    rstd_co_try(check_comment_scope(runtime, target, entry.f_scope.as_str()));
    if (page_content.is_none()) co_return Ok(unsupported_comment_replies(request_id));
    auto page     = rstd_co_try(comment_detail::comment_page_message(
        rstd::move(*page_content), target.scope.as_str(), query, parent));
    auto rsp      = Box<p_qcm::p_msg::M_GetCommentRepliesRsp>::make();
    rsp->f_result = Some(comments_result(
        true, target.scope.as_str(), target.provider_id, target.epoch, Some(rstd::move(page))));
    co_return Ok(qcm_message(request_id, rstd::move(rsp)));
}
auto set_comment_like_async(ProviderRuntime& runtime, sqlito::ExecutorHandle db,
                            SyncCoordinator& coordinator, i32 request_id,
                            const p_qcm::p_msg::M_SetCommentLikeReq& entry,
                            ProviderRequestOwnerHandle               owner = {})
    -> rstd::async::coro<Result<QcmMessage, ProcessError>> {
    if (entry.f_comment_id.is_empty() || entry.f_comment_id.len() > usize(256))
        co_return Err(ProcessError::MissingFields(String::make("invalid comment id"_str)));
    auto resolved = rstd_co_try(co_await resolve_comment_target(
        db.clone(), runtime, entry.f_id, i64(entry.f_item_type.to_primitive())));
    if (resolved.is_none()) co_return Err(ProcessError::NotImplemented());
    auto& target = *resolved;
    rstd_co_try(check_comment_scope(runtime, target, entry.f_scope.as_str()));
    auto loaded = co_await call_comment_provider<comment_model::CommentLikeResult>(
        runtime,
        db.clone(),
        coordinator,
        target,
        owner,
        [native_id  = target.native_id.clone(),
         item_type  = target.item_type,
         comment_id = entry.f_comment_id.clone(),
         value      = entry.f_value](Provider& instance, ProviderContext context) mutable {
            return instance.set_comment_like_async(
                rstd::move(context),
                comment_model::CommentTarget { rstd::move(native_id), item_type },
                rstd::move(comment_id),
                value);
        });
    auto reaction = rstd_co_try(rstd::move(loaded), comment_error);
    rstd_co_try(check_comment_scope(runtime, target, entry.f_scope.as_str()));
    if (reaction.liked != entry.f_value)
        co_return Err(ProcessError::internal("unexpected comment reaction"_str));
    auto result          = Box<p_qcm::p_msg::p_model::M_CommentLikeResult>::make();
    result->f_liked      = reaction.liked;
    result->f_like_count = reaction.like_count;
    auto rsp             = Box<p_qcm::p_msg::M_SetCommentLikeRsp>::make();
    rsp->f_result        = Some(rstd::move(result));
    co_return Ok(qcm_message(request_id, rstd::move(rsp)));
}
} // namespace qcm::backend
