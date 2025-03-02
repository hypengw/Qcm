#pragma once

#include <optional>
#include <nlohmann/json.hpp>

#include "helper.h"
#include "core/helper.h"

#define JSON_FROM_OPTIONAL(v1) from_json_opt(nlohmann_json_t.v1, nlohmann_json_j, #v1);

namespace
{

template<typename Type>
void from_json_opt(Type& out, const nlohmann::json& j, std::string_view name) {
    if constexpr (ycore::is_specialization_of_v<Type, std::optional>) {
        out = j.value<Type>(name, std::nullopt);
    } else {
        j.at(name).get_to(out);
    }
}

template<typename T, typename... Ts>
std::string variant_from_json(const nlohmann::json& j, std::variant<Ts...>& data) {
    try {
        data = j.get<T>();
        return {};
    } catch (const nlohmann::json::exception& e) {
        return e.what();
    }
}

} // namespace

#define JSON_GET_IMPL(Type)                                                                     \
    template auto qcm::json::detail::get_to<Type>(const qcm::json::njson& j, Type& v) -> Type&; \
    template auto qcm::json::detail::assign<Type>(qcm::json::njson & j, const Type& v) -> void;

#define JSON_GET_IMPL2(Type)                    \
    {                                           \
        Type* t { nullptr };                    \
        qcm::json::detail::get_to<Type>(j, *t); \
        qcm::json::detail::assign<Type>(j, *t); \
    }

#define JSON_DEFINE_IMPL(Type, ...)                                                \
    void to_json(nlohmann::json& nlohmann_json_j, const Type& nlohmann_json_t) {   \
        NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_TO, __VA_ARGS__))   \
    }                                                                              \
    void from_json(const nlohmann::json& nlohmann_json_j, Type& nlohmann_json_t) { \
        NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(JSON_FROM_OPTIONAL, __VA_ARGS__)) \
    }
#define JSON_DEFINE_WITH_DEFAULT_IMPL(Type, ...)                                                \
    void to_json(nlohmann::json& nlohmann_json_j, const Type& nlohmann_json_t) {                \
        NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_TO, __VA_ARGS__))                \
    }                                                                                           \
    void from_json(const nlohmann::json& nlohmann_json_j, Type& nlohmann_json_t) {              \
        const Type nlohmann_json_default_obj {};                                                \
        NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_FROM_WITH_DEFAULT, __VA_ARGS__)) \
    }

namespace qcm
{

template<typename ValueType>
auto json::detail::get_to(const njson& j, ValueType& v) -> ValueType& {
    j.get_to(v);
    return v;
}

template<typename ValueType>
void json::detail::assign(njson& j, const ValueType& v) {
    j = v;
}

} // namespace qcm

NLOHMANN_JSON_NAMESPACE_BEGIN
template<typename T>
struct adl_serializer<std::optional<T>> {
    static void to_json(json& j, const std::optional<T>& opt) {
        if (opt == std::nullopt) {
            j = nullptr;
        } else {
            j = *opt;
        }
    }
    static void from_json(const json& j, std::optional<T>& opt) {
        if (j.is_null())
            opt = std::nullopt;
        else
            opt = j.get<T>();
    }
};

template<typename... Ts>
struct adl_serializer<std::variant<Ts...>> {
    static void to_json(nlohmann::json& j, const std::variant<Ts...>& data) {
        // Will call j = v automatically for the right type
        std::visit(
            [&j](const auto& v) {
                j = v;
            },
            data);
    }

    static void from_json(const nlohmann::json& j, std::variant<Ts...>& data) {
        // Call variant_from_json for all types, only one will succeed
        std::array res { variant_from_json<Ts>(j, data)... };
        if (std::all_of(res.begin(), res.end(), [](const auto& x) {
                return ! x.empty();
            })) {
            throw nlohmann::json::type_error::create(
                302, fmt::format("{}", fmt::join(res, "\n")), nullptr);
        }
    }
};

template<typename T>
struct adl_serializer<rc<T>> {
    static void to_json(json& j, const rc<T>& opt) {
        if (opt) {
            j = *opt;
        } else {
            j = nullptr;
        }
    }
    static void from_json(const json& j, rc<T>& opt) {
        if (j.is_null()) {
            opt = nullptr;
        } else {
            if (! opt) opt = make_rc<T>();
            j.get_to(*opt);
        }
    }
};
NLOHMANN_JSON_NAMESPACE_END
