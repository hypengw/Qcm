#pragma once
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <variant>
#include <array>
#include <span>

#include <nlohmann/json_fwd.hpp>

#include "core/expected_helper.h"
#include "core/fmt.h"

#define JSON_GET(_j_, _v_, ...) qcm::json::get_to((_j_), qcm::json::make_keys(__VA_ARGS__), (_v_))

#define JSON_DEFINE(Type)                              \
    extern void to_json(nlohmann::json&, const Type&); \
    extern void from_json(const nlohmann::json&, Type&)

namespace qcm
{
namespace json
{
using njson = nlohmann::json;

struct Error {
    enum class Id
    {
        ParseError,
        TypeError,
        OutOfRange,
    };

    Id          id;
    std::string what;
};

using Key = std::variant<std::string_view, usize>;

namespace detail
{
void deleter(njson*);
}

using up_njson = up<njson, decltype(&detail::deleter)>;

template<typename... T>
constexpr auto make_keys(T&&... t) {
    return std::array { Key { t }... };
}

inline const njson& at_keys(const njson&, std::span<const Key>);

template<typename T>
extern nstd::expected<T, Error> get(const njson&, std::span<const Key>);

template<typename T>
std::optional<Error> get_to(const njson& j, std::span<const Key> k, T& v) {
    auto opt = get<T>(j, k);
    if (opt.has_value()) {
        v = opt.value();
        return std::nullopt;
    } else
        return opt.error();
}

nstd::expected<up_njson, Error> parse(std::string_view source);

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