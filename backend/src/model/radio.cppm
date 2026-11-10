module;
#include <rstd/macro.hpp>
export module qcm.backend:model.radio;
export import :model.song;
export import :model.item;
export import rstd;
export import rstd.serde;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
using QueueConfiguration = rstd::collections::BTreeMap<String, String>;

struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", rename("QueueInput"),
                                                         encode_only, ignore_unknown)]] QueueInput {
    String   native_id;
    ItemType item_type;
};
enum class [[pmacro::derive("serde::derive"),
             pmacro::helper("serde::derive", rename("QueueOpenReason"), string_enum,
                            encode_only)]] QueueOpenReason
{
    Initial,
    Reconfigure,
    Restart
};
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", rename("QueueOpenRequest"), encode_only, ignore_unknown)]]
QueueOpenRequest {
    QueueConfiguration configuration;
    Option<QueueInput> input;
    QueueOpenReason    reason { QueueOpenReason::Initial };
    Option<String>     previous_state;
};
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", rename("QueueNextRequest"), encode_only, ignore_unknown)]]
QueueNextRequest {
    String      state;
    Vec<String> current_native_ids;
};
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", rename("RadioConfigOption"), decode_only, ignore_unknown)]]
RadioConfigOption {
    String value;
    String name;
    [[pmacro::helper("serde::derive", default)]]
    String description;
};
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", rename("RadioConfigField"), decode_only, ignore_unknown)]]
RadioConfigField {
    String key;
    String name;
    [[pmacro::helper("serde::derive", default)]]
    String                 description;
    String                 value;
    Vec<RadioConfigOption> options;
};
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", rename("RadioDefinition"), decode_only, ignore_unknown)]]
RadioDefinition {
    [[pmacro::helper("serde::derive", default)]]
    Vec<RadioConfigField> configuration;
    [[pmacro::helper("serde::derive", default)]]
    bool can_restart {};
    [[pmacro::helper("serde::derive", default)]]
    String restart_label;
};
struct [[pmacro::derive("serde::derive", "rstd::Clone"),
         pmacro::helper("serde::derive", rename("QueuePresentation"), decode_only, ignore_unknown)]]
QueuePresentation {
    [[pmacro::helper("serde::derive", default)]]
    String description;
    [[pmacro::helper("serde::derive", default)]]
    Option<i64> source_item_id;

    auto clone() const -> QueuePresentation;
};
inline auto QueuePresentation::clone() const -> QueuePresentation {
    return rstd::as<rstd::clone::Clone>(*this).clone();
}
struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", rename("RadioBatch"),
                                                         decode_only, ignore_unknown)]] RadioBatch {
    Vec<Song> songs;
    [[pmacro::helper("serde::derive", default)]]
    String state;
    [[pmacro::helper("serde::derive", default)]]
    QueueConfiguration configuration;
    [[pmacro::helper("serde::derive", default)]]
    QueuePresentation presentation;
    [[pmacro::helper("serde::derive", default)]]
    bool exhausted {};
};
} // namespace qcm::backend
