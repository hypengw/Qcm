export module qcm.backend:provider.settings;
export import rstd;
export import rstd.json;
export import rstd.serde;
export import :error_cause;
export import :provider.error;
using rstd::collections::BTreeMap;
using rstd::sync::Arc;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{

enum class [[pmacro::derive("serde::derive"),
             pmacro::helper("serde::derive", rename("SettingKind"), string_enum)]] SettingKind
{
    Boolean [[pmacro::helper("serde::derive", rename("boolean"))]],
    Choice [[pmacro::helper("serde::derive", rename("choice"))]],
    Integer [[pmacro::helper("serde::derive", rename("integer"))]],
    Text [[pmacro::helper("serde::derive", rename("text"))]],
};

struct [[pmacro::derive("serde::derive", "rstd::Clone"),
         pmacro::helper("serde::derive", ignore_unknown)]] SettingOption {
    rstd::json::Value value;
    String            name;

    auto clone() const -> SettingOption;
    void clone_from(const SettingOption& source);
};

struct [[pmacro::derive("serde::derive", "rstd::Clone"),
         pmacro::helper("serde::derive", ignore_unknown)]] SettingCondition {
    String            key;
    rstd::json::Value value;

    auto clone() const -> SettingCondition;
    void clone_from(const SettingCondition& source);
};

struct [[pmacro::derive("serde::derive", "rstd::Clone"),
         pmacro::helper("serde::derive", ignore_unknown)]] SettingField {
    String key;
    String name;
    [[pmacro::helper("serde::derive", default)]]
    String group;
    [[pmacro::helper("serde::derive", default)]]
    String            description;
    SettingKind       kind;
    rstd::json::Value default_value;
    [[pmacro::helper("serde::derive", default)]]
    Vec<SettingOption> options;
    [[pmacro::helper("serde::derive", default)]]
    Option<i64> minimum;
    [[pmacro::helper("serde::derive", default)]]
    Option<i64> maximum;
    [[pmacro::helper("serde::derive", default)]]
    Option<SettingCondition> enabled_when;

    auto clone() const -> SettingField;
    void clone_from(const SettingField& source);
};

using SettingsValues = rstd::json::Map;

struct [[pmacro::derive("serde::derive", "rstd::Clone"),
         pmacro::helper("serde::derive", ignore_unknown)]] SettingsSnapshot {
    u64            revision {};
    SettingsValues values;

    auto clone() const -> SettingsSnapshot;
    void clone_from(const SettingsSnapshot& source);
};

struct [[pmacro::derive("rstd::Clone", "rstd::Debug", "rstd::Error"),
         pmacro::helper("rstd::Error", format("{message}"))]] SettingsError {
    String                   message;
    BTreeMap<String, String> fields;
    [[pmacro::helper("rstd::Error", source)]]
    Option<Arc<ErrorCause>> cause;

    auto clone() const -> SettingsError;
    void clone_from(const SettingsError& source);
};
} // namespace qcm::backend

export namespace qcm::backend
{
auto validate_settings(slice<SettingField> fields, const SettingsValues& values)
    -> Result<SettingsValues, SettingsError>;
} // namespace qcm::backend

export namespace rstd
{
template<>
struct Impl<convert::From<qcm::backend::SettingsError>, qcm::backend::ProviderError> {
    static auto from(qcm::backend::SettingsError error) -> qcm::backend::ProviderError;
};
} // namespace rstd
