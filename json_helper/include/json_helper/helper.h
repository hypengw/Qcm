#pragma once
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <variant>
#include <array>
#include <span>
#include <filesystem>

#include <nlohmann/json_fwd.hpp>

#include "core/expected_helper.h"
#include "core/fmt.h"

#define JSON_GET(_j_, _v_, ...) qcm::json::get_to((_j_), qcm::json::make_keys(__VA_ARGS__), (_v_))

#define JSON_DEFINE(Type)                              \
    extern void to_json(nlohmann::json&, const Type&); \
    extern void from_json(const nlohmann::json&, Type&)

#define DECLARE_JSON_SERIALIZER(Type, ...)                     \
    NLOHMANN_JSON_NAMESPACE_BEGIN                              \
    template<>                                                 \
    struct __VA_ARGS__ adl_serializer<Type> {                  \
        static void to_json(qcm::json::njson&, const Type&);   \
        static void from_json(const qcm::json::njson&, Type&); \
    };                                                         \
    NLOHMANN_JSON_NAMESPACE_END

#define JSON_SERIALIZER_NAMESPACE_BEGIN NLOHMANN_JSON_NAMESPACE_BEGIN
#define JSON_SERIALIZER_NAMESPACE_END   NLOHMANN_JSON_NAMESPACE_END

#define IMPL_JSON_SERIALIZER_FROM(Type)                                                      \
    void NLOHMANN_JSON_NAMESPACE::adl_serializer<Type>::from_json(const qcm::json::njson& j, \
                                                                  Type&                   t)

#define IMPL_JSON_SERIALIZER_TO(Type) \
    void NLOHMANN_JSON_NAMESPACE::adl_serializer<Type>::to_json(qcm::json::njson& j, const Type& t)

namespace qcm
{
namespace json
{
using njson = NLOHMANN_JSON_NAMESPACE::json;

template<typename T = void, typename SFINAE = void>
using adl_serializer = NLOHMANN_JSON_NAMESPACE::adl_serializer<T, SFINAE>;

struct Error {
    enum class Id
    {
        IoError = 0,
        ParseError,
        TypeError,
        OutOfRange,
    };

    Id          id;
    std::string what;
};

using Key = std::variant<std::string_view, usize>;
template<typename T>
using Result = nstd::expected<T, Error>;

namespace detail
{
void deleter(njson*);

template<typename>
struct is_basic_json : std::false_type {};

template<template<typename, typename, typename...> class ObjectType,
         template<typename, typename...> class ArrayType, class StringType, class BooleanType,
         class NumberIntegerType, class NumberUnsignedType, class NumberFloatType,
         template<typename> class AllocatorType,
         template<typename, typename = void> class JSONSerializer, class BinaryType,
         class CustomBaseClass>
struct is_basic_json<NLOHMANN_JSON_NAMESPACE::basic_json<
    ObjectType, ArrayType, StringType, BooleanType, NumberIntegerType, NumberUnsignedType,
    NumberFloatType, AllocatorType, JSONSerializer, BinaryType, CustomBaseClass>> : std::true_type {
};
template<typename T, typename... Args>
concept to_json_function = requires(Args... args) {
    { T::to_json(args...) } -> std::same_as<void>;
};
template<typename T, typename... Args>
concept from_json_function = requires(Args... args) {
    { T::from_json(args...) } -> std::same_as<void>;
};

template<typename BasicJsonType, typename T>
concept has_from_json = (! is_basic_json<T>::value) &&
                        from_json_function<adl_serializer<T, void>, const BasicJsonType&, T&>;

template<typename BasicJsonType, typename T>
concept has_to_json = (! is_basic_json<T>::value) &&
                      to_json_function<adl_serializer<T, void>, BasicJsonType&, const T&>;

template<typename ValueType>
auto get_to(const njson&, ValueType&) -> ValueType&;
template<typename ValueType>
void assign(njson&, const ValueType&);

template<typename ValueType>
    requires(! detail::is_basic_json<ValueType>::value) && detail::has_from_json<njson, ValueType>
auto get_to(const njson& j, ValueType& v) -> ValueType& {
    adl_serializer<ValueType>::from_json(j, v);
    return v;
}
template<typename ValueType>
    requires(! detail::is_basic_json<ValueType>::value) && detail::has_to_json<njson, ValueType>
void assign(njson& j, const ValueType& v) {
    adl_serializer<ValueType>::to_json(j, v);
}

} // namespace detail

using up_njson = up<njson, decltype(&detail::deleter)>;

template<typename... T>
constexpr auto make_keys(T&&... t) {
    return std::array { Key { t }... };
}

auto at_keys(const njson&, std::span<const Key>) -> const njson&;
auto assign_keys(njson&, std::span<const Key>) -> njson&;

auto catch_error(const std::function<void()>&) -> std::optional<Error>;

template<typename T>
auto get_to(const njson& j, T& out) -> std::optional<Error> {
    return catch_error([&j, &out] {
        detail::get_to(j, out);
    });
}

template<typename T>
auto get_to(const njson& j_in, std::span<const Key> keys, T& out) -> std::optional<Error> {
    return catch_error([&j_in, &keys, &out] {
        const njson& j = at_keys(j_in, keys);
        detail::get_to(j, out);
    });
}

template<typename T>
auto get(const njson& j_in) -> Result<T> {
    T    out;
    auto err = get_to(j_in, out);
    if (err)
        return nstd::unexpected(err.value());
    else
        return out;
}

template<typename T>
auto get(const njson& j_in, std::span<const Key> keys) -> Result<T> {
    T    out;
    auto err = get_to(j_in, keys, out);
    if (err)
        return nstd::unexpected(err.value());
    else
        return out;
}

template<typename T>
void assign(njson& j, const T& v) {
    detail::assign(j, v);
}

template<typename T>
void assign(njson& j_in, std::span<const Key> keys, const T& v) {
    auto& j = assign_keys(j_in, keys);
    assign(j, v);
}

auto create() -> up_njson;
auto parse(const std::filesystem::path&) -> Result<up_njson>;
auto parse(std::string_view source) -> Result<up_njson>;

} // namespace json
} // namespace qcm

DECLARE_CONVERT(std::string, qcm::json::njson)

template<>
struct fmt::formatter<qcm::json::Error> : fmt::formatter<std::string> {
    template<typename FormatContext>
    auto format(const qcm::json::Error& e, FormatContext& ctx) const {
        auto s = fmt::format("{}({})", e.what, (int)e.id);
        return fmt::formatter<std::string>::format(s, ctx);
    }
};