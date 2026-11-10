module;
#include <rstd/macro.hpp>
export module qcm.backend:fts;
export import liunicode;
export import sqlito;
import liorm;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend
{
enum class TokenKind
{
    Alphabetic,
    Numeric,
    Other,
    Separator
};
struct FtsToken {
    String    text;
    usize     begin {}, end {};
    TokenKind kind {};
};
struct FtsBlock {
    usize first {}, last {}, begin {}, end {};
};
auto token_kind(unsigned cp) -> TokenKind {
    if ((cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z')) return TokenKind::Alphabetic;
    if (cp >= '0' && cp <= '9') return TokenKind::Numeric;
    return cp < 0x80 ? TokenKind::Separator : TokenKind::Other;
}
auto tokenize_fts(slice<u8> input, liunicode::NormalizationControl control = {})
    -> Result<Vec<FtsToken>, liunicode::Error> {
    auto cancelled = [&] {
        return control.cancelled && control.cancelled(control.context);
    };
    if (cancelled()) return Err(liunicode::Error::Cancelled);
    if (input.is_empty()) return Ok(Vec<FtsToken> {});
    usize size;
    while (size < input.len() && input[size] != u8()) {
        if (cancelled()) return Err(liunicode::Error::Cancelled);
        ++size;
    }
    auto                         prefix = slice<u8>::from_raw_parts(input.as_raw_ptr(), size);
    auto                         chars  = rstd::str_::utf8_lossy_indices(prefix);
    Vec<liunicode::SourceScalar> source;
    for (auto cp = chars.next(); cp.is_some(); cp = chars.next()) {
        if (cancelled()) return Err(liunicode::Error::Cancelled);
        source.push(
            liunicode::SourceScalar { unsigned(cp->value.to_primitive()), cp->begin, cp->end });
    }
    auto          normalized = rstd_try(liunicode::normalize_with_source(
        source.as_slice(), liunicode::Normalization::NFKC, control));
    Vec<FtsToken> tokens;
    auto          add = [&](usize first, usize last, TokenKind kind) -> bool {
        auto text  = String::make();
        auto begin = normalized[first].begin;
        auto end   = normalized[first].end;
        for (auto n = first; n < last; ++n) {
            if (cancelled()) return false;
            auto cp = normalized[n].code_point;
            if (kind == TokenKind::Alphabetic && cp >= 'A' && cp <= 'Z') cp += 'a' - 'A';
            text.push(char32_t(cp));
            if (normalized[n].begin < begin) begin = normalized[n].begin;
            if (normalized[n].end > end) end = normalized[n].end;
        }
        tokens.push(FtsToken { rstd::move(text), begin, end, kind });
        return true;
    };
    for (usize first {}; first < normalized.len();) {
        if (cancelled()) return Err(liunicode::Error::Cancelled);
        auto kind = token_kind(normalized[first].code_point);
        auto last = first + usize(1);
        while (last < normalized.len() && token_kind(normalized[last].code_point) == kind) {
            if (cancelled()) return Err(liunicode::Error::Cancelled);
            ++last;
        }
        if (kind == TokenKind::Other && last - first > usize(1)) {
            for (auto index = first; index + usize(1) < last; ++index)
                if (! add(index, index + usize(2), kind)) return Err(liunicode::Error::Cancelled);
        } else if (kind != TokenKind::Separator)
            if (! add(first, last, kind)) return Err(liunicode::Error::Cancelled);
        first = last;
    }
    Vec<FtsBlock> blocks;
    for (usize index {}; index < tokens.len(); ++index) {
        if (cancelled()) return Err(liunicode::Error::Cancelled);
        blocks.push(FtsBlock { index, index + usize(1), tokens[index].begin, tokens[index].end });
        while (blocks.len() > usize(1)) {
            if (cancelled()) return Err(liunicode::Error::Cancelled);
            auto current  = blocks[blocks.len() - usize(1)];
            auto previous = blocks[blocks.len() - usize(2)];
            if (previous.begin <= current.begin && previous.end <= current.end) break;
            (void)blocks.pop();
            auto& merged = blocks[blocks.len() - usize(1)];
            merged.last  = current.last;
            if (current.begin < merged.begin) merged.begin = current.begin;
            if (current.end > merged.end) merged.end = current.end;
        }
    }
    for (auto block : blocks.as_slice())
        for (auto index = block.first; index < block.last; ++index) {
            if (cancelled()) return Err(liunicode::Error::Cancelled);
            tokens[index].begin = block.begin;
            tokens[index].end   = block.end;
        }
    return Ok(rstd::move(tokens));
}
auto qcm_fts_factory() -> sqlito::FtsFactory {
    return sqlito::FtsFactory::make([](slice<sqlito::FtsArgument> args) noexcept
                                        -> Result<sqlito::FtsTokenizer, sqlito::FtsFailure> {
        if (! args.is_empty())
            return Err(
                sqlito::FtsFailure { sqlito::ErrorKind::Input,
                                     0,
                                     String::make("qcm tokenizer does not accept arguments"_str) });
        return Ok(sqlito::FtsTokenizer::make(
            [](const sqlito::FtsInput& input,
               sqlito::FtsTokenSink&   sink) noexcept -> Result<empty, sqlito::FtsFailure> {
                if (input.cancelled())
                    return Err(sqlito::FtsFailure {
                        sqlito::ErrorKind::Cancelled, 0, String::make("operation cancelled"_str) });
                liunicode::NormalizationControl control {
                    &input,
                    [](const void* value) noexcept -> bool {
                        return static_cast<const sqlito::FtsInput*>(value)->cancelled();
                    }
                };
                auto result = tokenize_fts(input.text(), control);
                if (result.is_err()) {
                    auto error = rstd::move(result).unwrap_err();
                    return Err(sqlito::FtsFailure { error == liunicode::Error::Cancelled
                                                        ? sqlito::ErrorKind::Cancelled
                                                        : sqlito::ErrorKind::Input,
                                                    0,
                                                    String::make("qcm tokenization failed"_str) });
                }
                auto tokens  = rstd::move(result).unwrap();
                bool emitted = false;
                // Rust treats query tokens as synonyms, including across phrase callbacks.
                // SQLite requires each callback's first token to be non-colocated.
                bool query = input.mode() == sqlito::FtsMode::Query ||
                             input.mode() == sqlito::FtsMode::QueryPrefix;
                for (const auto& token : tokens.as_slice()) {
                    if (input.cancelled())
                        return Err(sqlito::FtsFailure { sqlito::ErrorKind::Cancelled,
                                                        0,
                                                        String::make("operation cancelled"_str) });
                    rstd_try(sink.emit(
                        token.text.as_str().as_bytes(), token.begin, token.end, query && emitted));
                    emitted = true;
                }
                return Ok(empty {});
            }));
    });
}
auto qcm_fts_query_with_control(slice<u8> input, liunicode::NormalizationControl control)
    -> Result<String, liunicode::Error> {
    auto tokens = rstd_try(tokenize_fts(input, control));
    auto result = String::make();
    bool first  = true;
    for (const auto& token : tokens.as_slice()) {
        if (control.cancelled && control.cancelled(control.context))
            return Err(liunicode::Error::Cancelled);
        if (! first) result.push_str(" AND "_str);
        first      = false;
        auto chars = token.text.as_str()->chars();
        for (auto cp = chars.next(); cp.is_some(); cp = chars.next()) {
            if (control.cancelled && control.cancelled(control.context))
                return Err(liunicode::Error::Cancelled);
            result.push(cp->to_primitive());
        }
        if (token.kind != TokenKind::Numeric) result.push(U'*');
    }
    return Ok(rstd::move(result));
}
auto qcm_fts_scalar() -> sqlito::ScalarFunction {
    return sqlito::ScalarFunction::make([](const sqlito::ScalarInput& input) noexcept
                                            -> Result<Option<String>, sqlito::CallbackFailure> {
        auto value = input.value();
        if (value.is_none())
            return Err(sqlito::CallbackFailure {
                sqlito::ErrorKind::Input, 0, String::make("Query text is null"_str) });
        liunicode::NormalizationControl control {
            &input,
            [](const void* value) noexcept -> bool {
                return static_cast<const sqlito::ScalarInput*>(value)->cancelled();
            }
        };
        auto result = qcm_fts_query_with_control(*value, control);
        if (result.is_err()) {
            auto error = rstd::move(result).unwrap_err();
            return Err(sqlito::CallbackFailure { error == liunicode::Error::Cancelled
                                                     ? sqlito::ErrorKind::Cancelled
                                                     : sqlito::ErrorKind::Input,
                                                 0,
                                                 String::make("qcm query failed"_str) });
        }
        return Ok(Some(rstd::move(result).unwrap()));
    });
}
} // namespace qcm::backend

export namespace qcm::backend
{
inline constexpr auto qcm_fts_tables = liorm::schema::define([](liorm::schema::SchemaBuilder& b) {
    for (auto name : array<ref<str>, 3> { "album_fts"_str, "artist_fts"_str, "song_fts"_str }) {
        auto table = b.table(name);
        table.column<i64>("rowid"_str);
        // The hidden table-name column is a MATCH target, not a decoded field.
        table.column<liorm::query::TextParameter>(name);
    }
});

auto qcm_fts_query(slice<u8> input, liunicode::NormalizationControl control = {})
    -> Result<String, liunicode::Error> {
    return qcm_fts_query_with_control(input, control);
}
} // namespace qcm::backend
