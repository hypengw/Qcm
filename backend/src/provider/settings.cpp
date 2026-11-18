module;
#include <rstd/macro.hpp>
module qcm.backend;
import :provider.settings;
using rstd::collections::BTreeMap;
using rstd::sync::Arc;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend
{
auto SettingsSnapshot::clone() const -> SettingsSnapshot {
    return rstd::as<rstd::clone::Clone>(*this).clone();
}

void SettingsSnapshot::clone_from(const SettingsSnapshot& source) {
    rstd::as<rstd::clone::Clone>(*this).clone_from(source);
}

auto SettingsError::clone() const -> SettingsError {
    return rstd::as<rstd::clone::Clone>(*this).clone();
}

void SettingsError::clone_from(const SettingsError& source) {
    rstd::as<rstd::clone::Clone>(*this).clone_from(source);
}

auto SettingOption::clone() const -> SettingOption {
    return rstd::as<rstd::clone::Clone>(*this).clone();
}

void SettingOption::clone_from(const SettingOption& source) {
    rstd::as<rstd::clone::Clone>(*this).clone_from(source);
}

auto SettingCondition::clone() const -> SettingCondition {
    return rstd::as<rstd::clone::Clone>(*this).clone();
}

void SettingCondition::clone_from(const SettingCondition& source) {
    rstd::as<rstd::clone::Clone>(*this).clone_from(source);
}

auto SettingField::clone() const -> SettingField {
    return rstd::as<rstd::clone::Clone>(*this).clone();
}

void SettingField::clone_from(const SettingField& source) {
    rstd::as<rstd::clone::Clone>(*this).clone_from(source);
}
} // namespace qcm::backend

namespace qcm::backend::settings_detail
{
auto message_error(ref<str> message) -> SettingsError {
    return { String::make(message), {}, None() };
}

auto check_size(const SettingsValues& values) -> Result<empty, SettingsError> {
    auto encoded = rstd::json::encode(values);
    if (encoded.is_err()) {
        auto error  = message_error("Provider settings are too large"_str);
        error.cause = Some(Arc<ErrorCause>::make(error_cause(rstd::move(encoded).unwrap_err())));
        return Err(rstd::move(error));
    }
    if (encoded->len() > usize(16 * 1024))
        return Err(message_error("Provider settings are too large"_str));
    return Ok(empty {});
}

auto check_value(const SettingField& field, const rstd::json::Value& value) -> bool {
    switch (field.kind) {
    case SettingKind::Boolean: return value.is_Bool();
    case SettingKind::Text: return value.is_String() && value.as_str()->len() <= usize(4096);
    case SettingKind::Integer: {
        auto number = value.as_i64();
        if (number.is_none()) return false;
        auto n = *number;
        if (n > i64(9007199254740991) || n < i64(-9007199254740991)) return false;
        if (field.minimum.is_some() && n < *field.minimum) return false;
        if (field.maximum.is_some() && n > *field.maximum) return false;
        return true;
    }
    case SettingKind::Choice:
        if (! value.is_String() && ! value.is_Bool() && value.as_i64().is_none()) return false;
        for (const auto& option : field.options.as_slice())
            if (option.value == value) return true;
        return false;
    }
    return false;
}

constexpr auto value_error = "Value does not match the setting's type or allowed values"_str;
} // namespace qcm::backend::settings_detail

namespace qcm::backend
{
auto validate_settings(slice<SettingField> fields, const SettingsValues& values)
    -> Result<SettingsValues, SettingsError> {
    using namespace settings_detail;
    if (fields.len() > usize(64)) return Err(message_error("Provider settings are too large"_str));
    rstd_try(check_size(values));

    BTreeMap<String, const SettingField*> definitions;
    for (const auto& field : fields) {
        if (field.key.is_empty() || field.key.len() > usize(64) ||
            definitions.contains_key(field.key.as_str()))
            return Err(message_error("Invalid provider settings definition"_str));
        definitions.insert(field.key.clone(), rstd::addressof(field));
        if (field.minimum.is_some() && field.maximum.is_some() && *field.minimum > *field.maximum)
            return Err(message_error("Invalid integer range"_str));
        if (! check_value(field, field.default_value)) return Err(message_error(value_error));
    }
    for (const auto& field : fields) {
        if (field.enabled_when.is_none()) continue;
        const auto& condition  = *field.enabled_when;
        auto        dependency = definitions.get(condition.key.as_str());
        if (condition.key == field.key || dependency.is_none())
            return Err(message_error("Invalid settings condition"_str));
        if (! check_value(***dependency, condition.value)) return Err(message_error(value_error));
    }

    SettingsValues           result;
    BTreeMap<String, String> errors;
    for (auto key : values.keys()) {
        if (! definitions.contains_key(key->as_str()))
            errors.insert(key->clone(), String::make("Unknown setting"_str));
    }
    for (const auto& field : fields) {
        auto        provided = values.get(field.key.as_str());
        const auto& value    = provided.is_some() ? **provided : field.default_value;
        if (! check_value(field, value)) {
            errors.insert(field.key.clone(), String::make(value_error));
        } else {
            result.insert(field.key.clone(), value.clone());
        }
    }
    rstd_try(check_size(result));
    if (! errors.is_empty())
        return Err(SettingsError {
            String::make("Invalid provider settings"_str), rstd::move(errors), None() });
    return Ok(rstd::move(result));
}
} // namespace qcm::backend

namespace rstd
{
auto Impl<convert::From<qcm::backend::SettingsError>, qcm::backend::ProviderError>::from(
    qcm::backend::SettingsError error) -> qcm::backend::ProviderError {
    return qcm::backend::ProviderError::external(rstd::move(error));
}
} // namespace rstd
