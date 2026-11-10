module;
#include <rstd/macro.hpp>
export module qcm.backend:db.timestamp;
export import rstd;
export import rstd.serde;
using rstd::time::SystemTime;
using namespace rstd::prelude;
export namespace qcm::backend
{
class Timestamp {
    i64 millis_ {};

public:
    constexpr Timestamp() = default;
    static constexpr auto from_millis(i64 value) -> Timestamp {
        Timestamp timestamp;
        timestamp.millis_ = value;
        return timestamp;
    }
    constexpr auto as_millis() const -> i64 { return millis_; }
    static auto    now() -> Timestamp {
        auto value = SystemTime::now().as_unix_time();
        return from_millis(value.seconds * i64(1000) +
                           i64(value.nanoseconds.to_primitive() / 1000000));
    }
    constexpr bool operator==(const Timestamp&) const  = default;
    constexpr auto operator<=>(const Timestamp&) const = default;
};
struct TimestampDefinition {
    static constexpr bool serializable   = true;
    static constexpr bool deserializable = true;
    template<class S>
    static auto serialize(S& sink, Timestamp value) -> typename S::result_type {
        return sink.serialize_i64(value.as_millis());
    }
    template<class D>
    static auto deserialize(D& source) -> Result<Timestamp, typename D::error_type> {
        return Ok(Timestamp::from_millis(rstd_try(source.deserialize_i64())));
    }
};
constexpr auto rstd_serde_definition(Timestamp*) { return TimestampDefinition {}; }
auto           current_millis() -> i64 { return Timestamp::now().as_millis(); }
} // namespace qcm::backend
