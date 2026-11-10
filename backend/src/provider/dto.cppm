export module qcm.backend:provider.dto;
export import rstd;
export import rstd.serde;
export import :model.item;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
struct [[pmacro::derive("rstd::Clone", "serde::derive"),
         pmacro::helper("serde::derive", rename("Username"), ignore_unknown)]] AuthUsername {
    String username;
    String pw;
};
struct [[pmacro::derive("rstd::Clone", "serde::derive"),
         pmacro::helper("serde::derive", rename("Phone"), ignore_unknown)]] AuthPhone {
    String phone;
    String pw;
};
struct [[pmacro::derive("rstd::Clone", "serde::derive"),
         pmacro::helper("serde::derive", rename("Email"), ignore_unknown)]] AuthEmail {
    String email;
    String pw;
};
struct [[pmacro::derive("rstd::Clone", "serde::derive"),
         pmacro::helper("serde::derive", rename("Qr"), ignore_unknown)]] AuthQr {
    String key;
};
[[pmacro::invoke(
    "rstd::Enum",
    [[derive(Clone, "serde::derive")]] [[helper("serde::derive", rename("AuthMethod"), internal,
                                                tag("type"), ignore_unknown)]] enum AuthMethod {
        Username = AuthUsername,
        Phone    = AuthPhone,
        Email    = AuthEmail,
        Qr       = AuthQr,
    } members {
    public:
        auto clone() const -> AuthMethod;
    })]];
inline auto AuthMethod::clone() const -> AuthMethod {
    return rstd::as<rstd::clone::Clone>(*this).clone();
}
struct [[pmacro::derive("serde::derive", "rstd::Clone"),
         pmacro::helper("serde::derive", rename("AuthInfo"), ignore_unknown)]] AuthInput {
    String server_url;
    [[pmacro::helper("serde::derive", default)]]
    Option<AuthMethod> method;
    auto               clone() const -> AuthInput;
};
inline auto AuthInput::clone() const -> AuthInput {
    return rstd::as<rstd::clone::Clone>(*this).clone();
}
enum class ProviderSyncMode
{
    Legacy,
    Explicit
};
struct [[pmacro::derive("rstd::Clone")]] ProviderSyncConfig {
    ProviderSyncMode mode { ProviderSyncMode::Legacy };
    Vec<ItemType>    types;
    auto             clone() const -> ProviderSyncConfig;
};
inline auto ProviderSyncConfig::clone() const -> ProviderSyncConfig {
    return rstd::as<rstd::clone::Clone>(*this).clone();
}
enum class SyncProgressKind
{
    Album,
    Artist,
    Song
};
[[pmacro::invoke("rstd::Enum",
                 [[derive("serde::derive")]] [[helper("serde::derive", rename("AuthOutcome"),
                                                      adjacent, tag("type"), content("message"),
                                                      ignore_unknown)]] enum AuthOutcome {
                     Ok,
                     [[helper("serde::derive", ignore_unknown)]] Failed { message: String },
                     WrongPassword,
                     NoSuchUsername,
                     NoSuchEmail,
                     NoSuchPhone,
                     QrExpired,
                     QrWaitScan,
                     [[helper("serde::derive", ignore_unknown)]] QrWaitComform {
                         name: String,
                         avatar_url: String,
                         message: String,
                     },
                 })]];
enum class ProviderSyncOutcome
{
    Completed,
    NotAuthenticated
};
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", rename("QrInfo"), decode_only, ignore_unknown)]] QrInfo {
    String key;
    String url;
};

} // namespace qcm::backend
