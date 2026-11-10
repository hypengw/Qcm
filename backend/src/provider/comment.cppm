module;
#include <rstd/macro.hpp>
export module qcm.backend:provider.comment;
export import :provider.error;
export import :model.item;
export import rstd;
export import rstd.serde;
import rstd.json;
import lihttpto;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend::provider_comment
{
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", rename("CommentTarget"), encode_only, ignore_unknown)]]
CommentTarget {
    String   native_id;
    ItemType item_type;
};
enum class [[pmacro::derive("serde::derive"),
             pmacro::helper("serde::derive", rename("CommentOrder"), string_enum)]] CommentOrder
{
    Hot,
    Latest
};
struct [[pmacro::derive("serde::derive", "rstd::Clone"),
         pmacro::helper("serde::derive", rename("CommentPageRequest"), encode_only,
                        ignore_unknown)]] CommentPageRequest {
    CommentOrder   order { CommentOrder::Latest };
    Option<String> state;
    u32            limit {};
    auto           clone() const -> CommentPageRequest;
};
inline auto CommentPageRequest::clone() const -> CommentPageRequest {
    return rstd::as<rstd::clone::Clone>(*this).clone();
}
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", rename("CommentAuthor"), decode_only, ignore_unknown)]]
CommentAuthor {
    [[pmacro::helper("serde::derive", default)]]
    String id;
    [[pmacro::helper("serde::derive", default)]]
    String name;
    [[pmacro::helper("serde::derive", default)]]
    Option<String> avatar_url;
};
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", rename("CommentQuote"), decode_only, ignore_unknown)]]
CommentQuote {
    [[pmacro::helper("serde::derive", default)]]
    String id;
    [[pmacro::helper("serde::derive", default)]]
    CommentAuthor author;
    [[pmacro::helper("serde::derive", default)]]
    String content;
    [[pmacro::helper("serde::derive", default)]]
    bool deleted {};
};
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", rename("Comment"), decode_only, ignore_unknown)]] Comment {
    [[pmacro::helper("serde::derive", default)]]
    String id;
    [[pmacro::helper("serde::derive", default)]]
    CommentAuthor author;
    [[pmacro::helper("serde::derive", default)]]
    String content;
    [[pmacro::helper("serde::derive", default)]]
    Option<i64> time_ms;
    [[pmacro::helper("serde::derive", default)]]
    Option<bool> liked;
    [[pmacro::helper("serde::derive", default)]]
    Option<u64> like_count;
    [[pmacro::helper("serde::derive", default)]]
    Option<u64> reply_count;
    [[pmacro::helper("serde::derive", default)]]
    bool deleted {};
    [[pmacro::helper("serde::derive", default)]]
    Option<CommentQuote> quote;
    [[pmacro::helper("serde::derive", default)]]
    Vec<Comment> previews;
};
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", rename("CommentPage"), decode_only, ignore_unknown)]]
CommentPage {
    Vec<Comment>      comments;
    CommentOrder      order { CommentOrder::Latest };
    Vec<CommentOrder> orders;
    [[pmacro::helper("serde::derive", default)]]
    Option<u64> total;
    bool        has_more {};
    [[pmacro::helper("serde::derive", default)]]
    Option<String> next_state;
    bool           can_reply {};
    bool           can_like {};
};
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", rename("CommentLikeResult"), decode_only, ignore_unknown)]]
CommentLikeResult {
    bool liked {};
    [[pmacro::helper("serde::derive", default)]]
    Option<u64> like_count;
};

auto valid_author(const CommentAuthor& author) -> bool {
    if (author.avatar_url.is_none()) return true;
    auto url = lihttpto::Url::parse_http(author.avatar_url->as_str());
    if (url.is_err() || url->host().is_none()) return false;
    auto userinfo = url->userinfo();
    return userinfo.is_none() || userinfo->is_empty();
}
auto valid_comment(const Comment& comment, usize depth) -> bool {
    if (comment.id.is_empty() || comment.id.len() > usize(256) || ! valid_author(comment.author) ||
        (comment.time_ms.is_some() && *comment.time_ms < i64()) ||
        (comment.quote.is_some() && ! valid_author(comment.quote->author)) ||
        comment.previews.len() > usize(3) || (depth >= usize(1) && ! comment.previews.is_empty()))
        return false;
    for (const auto& preview : comment.previews.as_slice())
        if (! valid_comment(preview, depth + usize(1))) return false;
    return true;
}
auto validate_page(const CommentPage& page, const CommentPageRequest& request)
    -> ProviderResult<empty> {
    bool has_order = false;
    for (auto order : page.orders.as_slice())
        if (order == page.order) has_order = true;
    auto has_next = page.next_state.is_some() && ! page.next_state->is_empty();
    if (page.comments.len() > usize(request.limit.to_primitive()) || page.order != request.order ||
        ! has_order || page.has_more != has_next ||
        (page.next_state.is_some() &&
         (page.next_state->len() > usize(8192) ||
          (request.state.is_some() && *page.next_state == *request.state))) ||
        (page.has_more && page.comments.is_empty()))
        return Err(ProviderError::internal("invalid comment page"_str));
    for (const auto& comment : page.comments.as_slice())
        if (! valid_comment(comment, usize()))
            return Err(ProviderError::internal("invalid comment page"_str));
    return Ok(empty {});
}
auto parse_page(ref<str> text, const CommentPageRequest& request)
    -> ProviderResult<Option<CommentPage>> {
    auto page = rstd::json::decode<Option<CommentPage>>(text);
    if (page.is_err()) return Err(ProviderError::serde(rstd::move(page).unwrap_err()));
    if (page->is_some()) rstd_try(validate_page(**page, request));
    return Ok(rstd::move(*page));
}
auto parse_like_result(ref<str> text, bool expected) -> ProviderResult<CommentLikeResult> {
    auto result = rstd::json::decode<CommentLikeResult>(text);
    if (result.is_err()) return Err(ProviderError::serde(rstd::move(result).unwrap_err()));
    if (result->liked != expected)
        return Err(ProviderError::internal("unexpected comment reaction"_str));
    return Ok(rstd::move(*result));
}
} // namespace qcm::backend::provider_comment
