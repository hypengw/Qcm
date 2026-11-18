module;
#include <rstd/macro.hpp>
export module qcm.backend:db.query.filter_expressions;
export import :db.query.input;
export import :db.error.query;
export import liorm;
import liorm.sqlito;
import :db.timestamp;
using rstd::collections::BTreeMap;
using namespace rstd::prelude;
namespace q = liorm::query;

export namespace qcm::backend
{
struct QueryIdProjection {
    using value_type = i64;
    ref<str> null_message;

    auto decode(liorm::sqlito::Reader& reader) const -> QueryResult<i64>;
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

    auto number(i64 value) -> q::Expr;
    auto text(ref<str> value, bool contains) -> q::Expr;
    auto integer(q::Expr column, i64 value, NumberCondition op) -> Option<q::Expr>;
    auto string(q::Expr column, ref<str> value, TextCondition op) -> Option<q::Expr>;
    auto type(q::Expr column, i64 value, TypeCondition op) -> Option<q::Expr>;
    auto type(q::Expr column, ref<str> value, TypeCondition op) -> Option<q::Expr>;
    auto date(q::Expr column, Timestamp value, DateCondition op) -> Option<q::Expr>;
    auto year(q::Expr column, int value, NumberCondition op) -> Option<q::Expr>;
    void add(int group, Option<q::Expr> expr);
    auto grouped(const Vec<FilterLogic>& logics) -> Option<q::Expr>;

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
