module;
#include <rstd/macro.hpp>
#include <rstd/enum.hpp>
export module qcm.backend:home;
export import liorm;
export import sqlito;
export import :model.item;
export import :model.remote_mix;
import liorm.sqlito;
import :db.database;
import :model.item_decode;
import :db.schema;
using namespace rstd::prelude;
using namespace rstd::literals;
namespace radio_query = liorm::query;

export namespace qcm::backend
{
struct RadioRow {
    RemoteMix remote_mix;
    Item      item;
};
// clang-format off
[[pmacro::invoke(
    "rstd::Enum",
    [[derive(Debug, Error)]] enum RadioQueryError {
        [[error("{error}")]]
        Query([[from]] error : radio_query::Error),
        [[error("{error}")]]
        Storage([[from]] error : sqlito::Error),
        [[error("invalid radio row")]]
        InvalidRow,
        [[error("radio item not found")]]
        NotFound,
        [[error("radio query range exceeded")]]
        Range,
    })]];
// clang-format on
template<class T>
using RadioQueryResult = Result<T, RadioQueryError>;
static_assert(! rstd::mtp::copy<RadioQueryError>);
} // namespace qcm::backend

namespace qcm::backend
{
inline constexpr radio_query::ProjectionLayout<radio_query::ModelGroup<RemoteMix>,
                                               radio_query::ModelGroup<Item>>
     radio_projection;
auto radio_invalid(bool item = false) -> RadioQueryError {
    return item ? RadioQueryError::NotFound() : RadioQueryError::InvalidRow();
}
template<class T>
auto radio_column_result(Result<T, sqlito::Error> value, bool item) -> RadioQueryResult<T> {
    if (value.is_err()) {
        auto failure = rstd::move(value).unwrap_err();
        if (failure.kind() == sqlito::ErrorKind::Type) return Err(radio_invalid(item));
        return Err(rstd::into<RadioQueryError>(rstd::move(failure)));
    }
    return Ok(rstd::move(value).unwrap());
}

auto decode_radio_row(sqlito::Statement& stmt) -> RadioQueryResult<RadioRow> {
    auto reader = liorm::sqlito::Reader(stmt);
    auto remote = rstd_try(
        radio_column_result(liorm::decode_model(radio_projection.group<0>(), reader), false));
    auto item = rstd_try(radio_column_result(
        decode_item(stmt, usize(radio_projection.group<1>().offset), Some(ItemType::Radio)), true));
    return Ok(RadioRow { rstd::move(remote), rstd::move(item) });
}
} // namespace qcm::backend

export namespace qcm::backend
{
auto radio_rows(sqlito::Connection& db, slice<i64> provider_ids)
    -> RadioQueryResult<Vec<RadioRow>> {
    constexpr auto max_sql = rstd::u32::MAX.to_primitive();
    auto           size    = provider_ids.len().to_primitive();
    if (size > (max_sql - 2048ULL) / 13ULL) return Err(RadioQueryError::Range());
    auto limits =
        radio_query::ExpansionLimits { .parameters = usize(size), .sql = usize(size * 13 + 2048) };
    auto query     = rstd_try(radio_query::try_compile_expanded(
        [&](radio_query::QueryBuilder& b) {
            auto schema = b.use(database_tables);
            auto mix    = schema.table<RemoteMix>();
            auto item   = schema.table<Item>();
            auto ids    = b.param_group<i64>("provider_ids"_str, provider_ids.len());
            return b.select(radio_query::project(radio_projection, mix, item).as_slice())
                .from(mix.source())
                .join(item.source(),
                      b.on(schema.relation(db_relations::remote_mix_item), mix, item))
                .where(item.col<&Item::provider_id>().in(ids) &&
                       (item.col<&Item::type>() == b.integer(55)) &&
                       (mix.col<&RemoteMix::mix_type>() == b.text_value("queue"_str)))
                .order_by(mix.col<&RemoteMix::id>().asc());
        },
        limits));
    auto statement = rstd_try(db.prepare(query.sql()));
    auto group     = query.parameter_group("provider_ids"_str).unwrap();
    for (usize i {}; i < group.count; ++i)
        rstd_try(statement.bind(group.first + i, provider_ids[i]));
    Vec<RadioRow>           rows;
    Option<RadioQueryError> decode_error;
    // The source query fetches every row before reporting model decoding errors.
    while (rstd_try(statement.step()) == sqlito::Step::Row) {
        if (decode_error.is_some() && ! decode_error->is_NotFound()) continue;
        auto row = decode_radio_row(statement);
        if (row.is_err()) {
            auto error = rstd::move(row).unwrap_err();
            RSTD_MATCH(error) {
                RSTD_CASE(NotFound) {
                    if (decode_error.is_none()) decode_error = Some(rstd::move(error));
                }
                break;
                RSTD_CASE_DEFAULT { decode_error = Some(rstd::move(error)); }
            }
            continue;
        }
        rows.push(rstd::move(row).unwrap());
    }
    if (decode_error.is_some()) return Err(rstd::move(decode_error).unwrap());
    return Ok(rstd::move(rows));
}
class RadioStore {
    sqlito::ExecutorHandle handle_;
    static auto            execute(sqlito::ExecutorHandle handle, Vec<i64> ids)
        -> rstd::async::coro<RadioQueryResult<Vec<RadioRow>>> {
        co_return co_await database_run(rstd::move(handle),
                                        [ids = rstd::move(ids)](sqlito::Connection& db) {
                                            return qcm::backend::radio_rows(db, ids.as_slice());
                                        });
    }

public:
    explicit RadioStore(sqlito::ExecutorHandle handle): handle_(rstd::move(handle)) {}
    auto rows(Vec<i64> ids) const -> rstd::async::coro<RadioQueryResult<Vec<RadioRow>>> {
        return execute(handle_.clone(), rstd::move(ids));
    }
};
} // namespace qcm::backend
