export module qcm.backend:provider.comment;
export import :provider.error;
export import :model.item;
export import rstd;
export import rstd.serde;
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

auto valid_author(const CommentAuthor& author) -> bool;
auto valid_comment(const Comment& comment, usize depth) -> bool;
auto validate_page(const CommentPage& page, const CommentPageRequest& request)
    -> ProviderResult<empty>;
auto parse_page(ref<str> text, const CommentPageRequest& request)
    -> ProviderResult<Option<CommentPage>>;
auto parse_like_result(ref<str> text, bool expected) -> ProviderResult<CommentLikeResult>;
} // namespace qcm::backend::provider_comment
