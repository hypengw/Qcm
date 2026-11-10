module;
#include <rstd/enum.hpp>
export module qcm.backend:protocol.provider;
export import :protocol.types;
export import :provider.registry;
export import :model.subtitle;
export import :model.radio;
export import rstd;
using namespace rstd::prelude;

namespace qcm::backend::protocol
{
auto auth_string(String& value) -> String { return rstd::move(value); }
auto auth_string(const String& value) -> String { return value.clone(); }

template<class T>
auto auth_method(T& method) -> AuthMethod {
    RSTD_MATCH(method) {
        RSTD_CASE_PAYLOAD(username, value) {
            return AuthMethod::Username(auth_string(value->f_username), auth_string(value->f_pw));
        }
        RSTD_CASE_PAYLOAD(phone, value) {
            return AuthMethod::Phone(auth_string(value->f_phone), auth_string(value->f_pw));
        }
        RSTD_CASE_PAYLOAD(email, value) {
            return AuthMethod::Email(auth_string(value->f_email), auth_string(value->f_pw));
        }
        RSTD_CASE_PAYLOAD(qr, value) { return AuthMethod::Qr(auth_string(value->f_key)); }
    }
}
} // namespace qcm::backend::protocol

export namespace rstd
{
template<>
struct Impl<convert::From<p_qcm::p_msg::p_model::M_AuthInfo::C_o_method>,
            qcm::backend::AuthMethod> {
    static auto from(p_qcm::p_msg::p_model::M_AuthInfo::C_o_method value)
        -> qcm::backend::AuthMethod {
        return qcm::backend::protocol::auth_method(value);
    }
};

template<>
struct Impl<convert::From<qcm::backend::AuthMethod>,
            p_qcm::p_msg::p_model::M_AuthInfo::C_o_method> {
    static auto from(qcm::backend::AuthMethod value)
        -> p_qcm::p_msg::p_model::M_AuthInfo::C_o_method {
        using Method = p_qcm::p_msg::p_model::M_AuthInfo::C_o_method;
        RSTD_MATCH(value) {
            RSTD_CASE(Username, username, pw) {
                auto auth        = Box<p_qcm::p_msg::p_model::M_UsernameAuth>::make();
                auth->f_username = rstd::move(username);
                auth->f_pw       = rstd::move(pw);
                return Method::make_username(rstd::move(auth));
            }
            RSTD_CASE(Phone, phone, pw) {
                auto auth     = Box<p_qcm::p_msg::p_model::M_PhoneAuth>::make();
                auth->f_phone = rstd::move(phone);
                auth->f_pw    = rstd::move(pw);
                return Method::make_phone(rstd::move(auth));
            }
            RSTD_CASE(Email, email, pw) {
                auto auth     = Box<p_qcm::p_msg::p_model::M_EmailAuth>::make();
                auth->f_email = rstd::move(email);
                auth->f_pw    = rstd::move(pw);
                return Method::make_email(rstd::move(auth));
            }
            RSTD_CASE(Qr, key) {
                auto auth   = Box<p_qcm::p_msg::p_model::M_QrAuth>::make();
                auth->f_key = rstd::move(key);
                return Method::make_qr(rstd::move(auth));
            }
        }
    }
};

template<>
struct Impl<convert::From<Box<p_qcm::p_msg::p_model::M_AuthInfo>>, qcm::backend::AuthInput> {
    static auto from(Box<p_qcm::p_msg::p_model::M_AuthInfo> value) -> qcm::backend::AuthInput {
        qcm::backend::AuthInput result { rstd::move(value->f_server_url), None() };
        if (value->o_method.is_some())
            result.method =
                Some(rstd::into<qcm::backend::AuthMethod>(rstd::move(*value->o_method)));
        return result;
    }
};

template<>
struct Impl<convert::From<ref<p_qcm::p_msg::p_model::M_AuthInfo>>, qcm::backend::AuthInput> {
    static auto from(ref<p_qcm::p_msg::p_model::M_AuthInfo> value) -> qcm::backend::AuthInput {
        qcm::backend::AuthInput result { value->f_server_url.clone(), None() };
        if (value->o_method.is_some())
            result.method = Some(qcm::backend::protocol::auth_method(*value->o_method));
        return result;
    }
};

template<>
struct Impl<convert::From<qcm::backend::AuthInput>, Box<p_qcm::p_msg::p_model::M_AuthInfo>> {
    static auto from(qcm::backend::AuthInput value) -> Box<p_qcm::p_msg::p_model::M_AuthInfo> {
        auto result          = Box<p_qcm::p_msg::p_model::M_AuthInfo>::make();
        result->f_server_url = rstd::move(value.server_url);
        if (value.method.is_some())
            result->o_method = Some(rstd::into<p_qcm::p_msg::p_model::M_AuthInfo::C_o_method>(
                rstd::move(*value.method)));
        return result;
    }
};

template<>
struct Impl<convert::From<qcm::backend::AuthOutcome>, p_qcm::p_msg::p_model::E_AuthResult> {
    static auto from(qcm::backend::AuthOutcome value) -> p_qcm::p_msg::p_model::E_AuthResult {
        using Code = p_qcm::p_msg::p_model::E_AuthResult;
        RSTD_MATCH(value) {
            RSTD_CASE(Ok) { return Code::v_Ok; }
            RSTD_CASE(Failed) { return Code::v_Failed; }
            RSTD_CASE(WrongPassword) { return Code::v_WrongPassword; }
            RSTD_CASE(NoSuchUsername) { return Code::v_NoSuchUsername; }
            RSTD_CASE(NoSuchEmail) { return Code::v_NoSuchEmail; }
            RSTD_CASE(NoSuchPhone) { return Code::v_NoSuchPhone; }
            RSTD_CASE(QrExpired) { return Code::v_QrExpired; }
            RSTD_CASE(QrWaitScan) { return Code::v_QrWaitScan; }
            RSTD_CASE(QrWaitComform) { return Code::v_QrWaitComform; }
        }
    }
};

template<>
struct Impl<convert::From<qcm::backend::AuthOutcome>, Box<p_qcm::p_msg::M_AuthProviderRsp>> {
    static auto from(qcm::backend::AuthOutcome value) -> Box<p_qcm::p_msg::M_AuthProviderRsp> {
        auto result = Box<p_qcm::p_msg::M_AuthProviderRsp>::make();
        RSTD_MATCH(value) {
            RSTD_CASE(Failed, message) { result->f_message = rstd::move(message); }
            RSTD_CASE(QrWaitComform, name, avatar_url, message) {
                result->f_message       = rstd::move(message);
                result->f_qr_name       = rstd::move(name);
                result->f_qr_avatar_url = rstd::move(avatar_url);
            }
            RSTD_CASE_DEFAULT { break; }
        }
        result->f_code = i32(
            static_cast<int>(rstd::into<p_qcm::p_msg::p_model::E_AuthResult>(rstd::move(value))));
        return result;
    }
};

template<>
struct Impl<convert::From<qcm::backend::AuthOutcome>, Box<p_qcm::p_msg::M_UpdateProviderRsp>> {
    static auto from(qcm::backend::AuthOutcome value) -> Box<p_qcm::p_msg::M_UpdateProviderRsp> {
        auto result = Box<p_qcm::p_msg::M_UpdateProviderRsp>::make();
        if (value.is_Failed()) result->f_message = rstd::move(value.as_Failed().message);
        result->f_code = i32(
            static_cast<int>(rstd::into<p_qcm::p_msg::p_model::E_AuthResult>(rstd::move(value))));
        return result;
    }
};

template<>
struct Impl<convert::From<ref<qcm::backend::ProviderMetaInfo>>,
            Box<p_qcm::p_msg::p_model::M_ProviderMeta>> {
    static auto from(ref<qcm::backend::ProviderMetaInfo> value)
        -> Box<p_qcm::p_msg::p_model::M_ProviderMeta> {
        auto result              = Box<p_qcm::p_msg::p_model::M_ProviderMeta>::make();
        result->f_type_name      = value->type_name.clone();
        result->f_svg            = value->svg.clone();
        result->f_mutable        = value->is_mutable;
        result->f_is_script      = value->is_script;
        result->f_has_server_url = value->has_server_url;
        result->f_auth_types     = value->auth_types.clone();
        return result;
    }
};

template<>
struct Impl<convert::From<qcm::backend::SubtitleData>, Box<p_qcm::p_msg::p_model::M_Subtitle>> {
    static auto from(qcm::backend::SubtitleData value) -> Box<p_qcm::p_msg::p_model::M_Subtitle> {
        auto result = Box<p_qcm::p_msg::p_model::M_Subtitle>::make();
        for (auto& item : value.items) {
            auto subtitle     = Box<p_qcm::p_msg::p_model::M_SubtitleItem>::make();
            subtitle->f_start = item.start.unwrap_or(i64(-1));
            subtitle->f_end   = item.end.unwrap_or(i64(-1));
            subtitle->f_text  = rstd::move(item.text).unwrap_or(String::make());
            result->f_items.push(rstd::move(subtitle));
        }
        return result;
    }
};

template<>
struct Impl<convert::From<qcm::backend::RadioConfigOption>,
            Box<p_qcm::p_msg::p_model::M_QueueConfigOption>> {
    static auto from(qcm::backend::RadioConfigOption value)
        -> Box<p_qcm::p_msg::p_model::M_QueueConfigOption> {
        auto result           = Box<p_qcm::p_msg::p_model::M_QueueConfigOption>::make();
        result->f_value       = rstd::move(value.value);
        result->f_name        = rstd::move(value.name);
        result->f_description = rstd::move(value.description);
        return result;
    }
};

template<>
struct Impl<convert::From<qcm::backend::RadioConfigField>,
            Box<p_qcm::p_msg::p_model::M_QueueConfigField>> {
    static auto from(qcm::backend::RadioConfigField value)
        -> Box<p_qcm::p_msg::p_model::M_QueueConfigField> {
        auto result           = Box<p_qcm::p_msg::p_model::M_QueueConfigField>::make();
        result->f_key         = rstd::move(value.key);
        result->f_name        = rstd::move(value.name);
        result->f_description = rstd::move(value.description);
        result->f_value       = rstd::move(value.value);
        for (auto& option : value.options)
            result->f_options.push(
                rstd::into<Box<p_qcm::p_msg::p_model::M_QueueConfigOption>>(rstd::move(option)));
        return result;
    }
};
} // namespace rstd
