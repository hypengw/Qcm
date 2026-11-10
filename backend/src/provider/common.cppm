export module qcm.backend:provider.common;
export import :provider.dto;
export import :provider.settings;
using namespace rstd::prelude;

export namespace qcm::backend
{
struct ProviderCommonSnapshot {
    String    name;
    String    type_name;
    AuthInput auth;
};

class ProviderCommonData {
    String             type_name_;
    String             name_;
    String             base_url_;
    SettingsSnapshot   settings_;
    Option<AuthMethod> auth_method_;
    String             device_id_;
    Option<i64>        id_;

public:
    ProviderCommonData(String type_name, String name, String base_url, SettingsSnapshot settings,
                       Option<AuthMethod> auth_method, String device_id, Option<i64> id)
        : type_name_(rstd::move(type_name)),
          name_(rstd::move(name)),
          base_url_(rstd::move(base_url)),
          settings_(rstd::move(settings)),
          auth_method_(rstd::move(auth_method)),
          device_id_(rstd::move(device_id)),
          id_(id) {}
    auto type_name() const -> ref<str> { return type_name_.as_str(); }
    auto name() const -> ref<str> { return name_.as_str(); }
    auto base_url() const -> ref<str> { return base_url_.as_str(); }
    auto settings() const -> const SettingsSnapshot& { return settings_; }
    auto auth_method() const -> const Option<AuthMethod>& { return auth_method_; }
    auto device_id() const -> ref<str> { return device_id_.as_str(); }
    auto id() const -> Option<i64> { return id_; }
    auto status_snapshot() const -> ProviderCommonSnapshot {
        return { name_.clone(),
                 type_name_.clone(),
                 AuthInput { base_url_.clone(),
                             auth_method_.is_some() ? Some(auth_method_->clone()) : None() } };
    }
    auto set_name(ref<str> name) -> void { name_ = String::make(name); }
    auto set_identity(i64 id, ref<str> name) -> void {
        id_ = Some(id);
        set_name(name);
    }
    auto set_auth_info(AuthInput input) -> void {
        base_url_    = rstd::move(input.server_url);
        auth_method_ = rstd::move(input.method);
    }
    auto set_settings(SettingsSnapshot settings) -> void { settings_ = rstd::move(settings); }
};
} // namespace qcm::backend
