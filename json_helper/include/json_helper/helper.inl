#pragma once

#include <optional>
#include <nlohmann/json.hpp>

#include "helper.h"
#include "core/variant_helper.h"

#define JSON_FROM_OPTIONAL(v1) from_json_opt(nlohmann_json_t.v1, nlohmann_json_j, #v1);

namespace
{

template<typename Type>
void from_json_opt(Type& out, const nlohmann::json& j, std::string_view name) {
    if constexpr (core::is_specialization_of<Type, std::optional>) {
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

#define JSON_GET_IMPL(_TYPE_)                                                 \
    template nstd::expected<_TYPE_, qcm::json::Error> qcm::json::get<_TYPE_>( \
        const qcm::json::njson&, std::span<const qcm::json::Key>)

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
namespace json
{

template<typename T>
nstd::expected<T, Error> get(const njson& j_in, std::span<const Key> keys) {
    using ET = Error::Id;
    try {
        const njson& j = at_keys(j_in, keys);
        return j.get<T>();
    } catch (const njson::type_error& e) {
        return nstd::unexpected(Error { .id = ET::TypeError, .what = e.what() });
    } catch (const njson::parse_error& e) {
        return nstd::unexpected(Error { .id = ET::ParseError, .what = e.what() });
    } catch (const njson::out_of_range& e) {
        return nstd::unexpected(Error { .id = ET::OutOfRange, .what = e.what() });
    }
}

} // namespace json

const json::njson& json::at_keys(const njson& j_in, std::span<const Key> keys) {
    return *std::accumulate(keys.begin(), keys.end(), &j_in, [](const njson* j, const Key& key) {
        const njson* out;
        std::visit(overloaded { [&out, j](std::string_view k) {
                                   out = &((*j).at(k));
                               },
                                [&out, j](usize idx) {
                                    out = &((*j).at(idx));
                                } },
                   key);
        return out;
    });
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
NLOHMANN_JSON_NAMESPACE_END
