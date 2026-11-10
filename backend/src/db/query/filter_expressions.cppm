module;
#include <rstd/macro.hpp>
export module qcm.backend:db.query.filter_expressions;
export import :db.query.input;
export import :db.error.query;
export import liorm;
import sqlito;
import liorm.sqlito;
import :datetime;
import :db.timestamp;
using rstd::collections::BTreeMap;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace q = liorm::query;

export namespace qcm::backend
{
struct QueryIdProjection {
    using value_type = i64;
    ref<str> null_message;

    auto decode(liorm::sqlito::Reader& reader) const -> QueryResult<i64> {
        auto value = rstd_try(reader.read<Option<i64>>(usize()));
        if (value.is_none()) return Err(QueryError::invalid(null_message));
        return Ok(*value);
    }
};

class FilterExpressions {
    struct Parameter {
        String         name;
        Option<String> text;
        i64            integer {};
    };
    Vec<Parameter>         parameters_;
    BTreeMap<i32, q::Expr> groups_;

protected:
    q::QueryBuilder& builder_;

    auto number(i64 value) -> q::Expr {
        auto name = rstd::format("filter_{}", parameters_.len());
        auto expr = builder_.param<i64>(name.as_str());
        parameters_.push({ rstd::move(name), None(), value });
        return expr;
    }
    auto text(ref<str> value, bool contains) -> q::Expr {
        auto name = rstd::format("filter_{}", parameters_.len());
        auto expr = builder_.param<q::TextParameter>(name.as_str());
        auto data = String::make();
        if (contains) data.push_ascii('%');
        data.push_str(value);
        if (contains) data.push_ascii('%');
        parameters_.push({ rstd::move(name), Some(rstd::move(data)), i64() });
        return expr;
    }
    auto integer(q::Expr column, i64 value, NumberCondition op) -> Option<q::Expr> {
        if (op == NumberCondition::Unspecified) return None();
        auto v = number(value);
        switch (op) {
        case NumberCondition::Equal: return Some(column == v);
        case NumberCondition::EqualNot: return Some(column != v);
        case NumberCondition::Less: return Some(column < v);
        case NumberCondition::LessEqual: return Some(column <= v);
        case NumberCondition::Greater: return Some(column > v);
        case NumberCondition::GreaterEqual: return Some(column >= v);
        case NumberCondition::Unspecified: return None();
        }
        return None();
    }
    auto string(q::Expr column, ref<str> value, TextCondition op) -> Option<q::Expr> {
        if (op == TextCondition::Unspecified) return None();
        auto v = text(value, op == TextCondition::Contains || op == TextCondition::ContainsNot);
        switch (op) {
        case TextCondition::Is: return Some(column == v);
        case TextCondition::IsNot: return Some(column != v);
        case TextCondition::Contains: return Some(column.like(v));
        case TextCondition::ContainsNot: return Some(! column.like(v));
        case TextCondition::Unspecified: return None();
        }
        return None();
    }
    auto type(q::Expr column, i64 value, TypeCondition op) -> Option<q::Expr> {
        switch (op) {
        case TypeCondition::Is: return Some(column == number(value));
        case TypeCondition::IsNot: return Some(column != number(value));
        case TypeCondition::Unspecified: return None();
        }
        return None();
    }
    auto type(q::Expr column, ref<str> value, TypeCondition op) -> Option<q::Expr> {
        switch (op) {
        case TypeCondition::Is: return Some(column == text(value, false));
        case TypeCondition::IsNot: return Some(column != text(value, false));
        case TypeCondition::Unspecified: return None();
        }
        return None();
    }
    auto date(q::Expr column, Timestamp value, DateCondition op) -> Option<q::Expr> {
        switch (op) {
        case DateCondition::Before: return Some(column <= number(value.as_millis()));
        case DateCondition::After: return Some(column >= number(value.as_millis()));
        case DateCondition::Null: return Some(column.is_null());
        case DateCondition::NotNull: return Some(column.is_not_null());
        case DateCondition::Unspecified: return None();
        }
        return None();
    }
    auto year(q::Expr column, int value, NumberCondition op) -> Option<q::Expr> {
        if (value < -32767 || value >= 32767 || op == NumberCondition::Unspecified) return None();
        auto start =
            *rstd::time::days_from_calendar_date(i32(value), rstd::time::Month::January, u8(1)) *
            i64(86400000);
        if (op == NumberCondition::Equal || op == NumberCondition::EqualNot) {
            auto next = *rstd::time::days_from_calendar_date(
                            i32(value + 1), rstd::time::Month::January, u8(1)) *
                        i64(86400000);
            auto a    = number(i64(start));
            auto b    = number(i64(next));
            return Some(op == NumberCondition::Equal ? (column >= a && column < b)
                                                     : (column < a || column >= b));
        }
        return integer(column, i64(start), op);
    }
    void add(int group, Option<q::Expr> expr) {
        if (! expr) return;
        auto previous = groups_.get(i32(group));
        auto combined = previous.is_some() ? **previous && *expr : *expr;
        groups_.insert(i32(group), combined);
    }
    auto grouped(const Vec<FilterLogic>& logics) -> Option<q::Expr> {
        Option<q::Expr>     result;
        BTreeMap<i32, bool> referenced;
        auto                append = [&](q::Expr expr) {
            result = Some(result ? *result && expr : expr);
        };
        for (const auto& logic : logics.as_slice()) {
            auto a    = logic.group_a;
            auto b    = logic.group_b;
            auto left = groups_.get(a), right = groups_.get(b);
            if (left.is_none() || right.is_none()) continue;
            append(logic.operation == LogicOp::Or ? **left || **right : **left && **right);
            referenced.insert(a, true);
            referenced.insert(b, true);
        }
        for (auto entry : groups_.iter())
            if (! referenced.contains_key(*rstd::get<0>(entry))) append(*rstd::get<1>(entry));
        return result;
    }

public:
    explicit FilterExpressions(q::QueryBuilder& builder): builder_(builder) {}
    template<class Prepared>
    auto bind(Prepared& stmt) const -> QueryResult<empty> {
        for (const auto& param : parameters_) {
            if (param.text.is_some()) {
                rstd_try(stmt.bind(param.name.as_str(), *param.text));
            } else {
                rstd_try(stmt.bind(param.name.as_str(), param.integer));
            }
        }
        return Ok(empty {});
    }
};
} // namespace qcm::backend
