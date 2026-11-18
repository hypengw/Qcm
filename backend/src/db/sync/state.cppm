export module qcm.backend:db.sync.state;
export import rstd;
import :datetime;
import :db.timestamp;
import sqlito;
using rstd::collections::BTreeMap;
using rstd::collections::HashSet;
using namespace rstd::prelude;

namespace qcm::backend
{
enum class ItemType : int;
class SyncGuard;
} // namespace qcm::backend

export namespace qcm::backend
{
struct ObserveScope {
    Option<i64> library_id;
    i64         item_type {};
    bool        complete {};
};
class SyncRunState {
    struct ScopeKey {
        bool library;
        i64  library_id;
        i64  item_type;
        auto operator<(const ScopeKey& other) const -> bool {
            if (library != other.library) return library < other.library;
            if (library_id != other.library_id) return library_id < other.library_id;
            return item_type < other.item_type;
        }
    };
    i64                       provider_ {};
    Vec<ObserveScope>         scopes_;
    BTreeMap<ScopeKey, usize> scope_index_;
    HashSet<i64>              seen_;
    Vec<i64>                  libraries_;
    HashSet<i64>              library_index_;
    bool                      explicit_ {};
    bool                      declared_ {};
    bool                      libraries_complete_ {};
    bool                      sealed_ {};
    bool                      failed_ {};
    DatabaseDateTime          started_ {};
    Timestamp                 started_millis_ {};

    auto find_scope(Option<i64> library_id, i64 item_type) -> Option<usize>;
    auto add_scope(ObserveScope scope) -> void;

    auto reject(ref<str> message) -> Result<empty, sqlito::Error>;

    auto has_library(i64 id) const -> bool;

public:
    auto reset(i64 provider, bool explicit_mode = false) -> void;
    auto provider() const -> i64;
    auto check_explicit_write() -> Result<empty, sqlito::Error>;
    auto observe_libraries(sqlito::Connection& db, slice<i64> libraries)
        -> Result<empty, sqlito::Error>;
    auto declare_explicit(slice<ObserveScope> scopes, slice<ItemType> required)
        -> Result<empty, sqlito::Error>;
    auto begin_explicit(sqlito::Connection& db, slice<i64> libraries, slice<ObserveScope> scopes,
                        slice<ItemType> required) -> Result<empty, sqlito::Error>;
    auto check_explicit_scope(Option<i64> library_id, i64 item_type)
        -> Result<empty, sqlito::Error>;
    auto observe_explicit(Option<i64> library_id, i64 item_type, slice<i64> items)
        -> Result<empty, sqlito::Error>;
    auto complete_explicit(Option<i64> library_id, i64 item_type) -> Result<empty, sqlito::Error>;
    auto complete_explicit_libraries() -> Result<empty, sqlito::Error>;
    auto seal_explicit() -> Result<empty, sqlito::Error>;
    auto finalize(sqlito::Connection& db, const SyncGuard& guard, slice<i64> retained)
        -> Result<u32, sqlito::Error>;
    auto observe(Option<i64> library_id, i64 item_type, i64 item_id) -> void;
    auto complete(Option<i64> library_id, i64 item_type) -> void;
    auto complete_libraries() -> void;
    auto seal() -> void;
    auto fail() -> void;
    auto finalize_legacy(sqlito::Connection& db, const SyncGuard& guard, slice<i64> retained)
        -> Result<u32, sqlito::Error>;
    auto prune(sqlito::Connection& db) -> Result<u32, sqlito::Error>;
};
} // namespace qcm::backend
