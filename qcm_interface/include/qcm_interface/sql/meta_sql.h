#pragma once

#include <ranges>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>

#include "core/str_helper.h"

namespace qcm::db
{

enum class Logical
{
    AND = 0,
    OR,
};

enum class Eq
{
    NOT = 0,
    EQ
};
using enum Logical;
using enum Eq;

constexpr auto suffix(bool v, std::string_view in, std::string_view suffix_) -> std::string {
    return v ? std::string(in) : fmt::format("{}{}", in, suffix_);
}
constexpr auto suffix(std::string_view in, std::string_view suffix_) -> std::string {
    return suffix(in.empty(), in, suffix_);
}

template<Logical op, Eq eq, std::ranges::range R>
auto null(const R& r, std::string_view table = {}) -> std::string {
    std::string_view x;

    return fmt::format(
        "{}",
        fmt::join(
            std::views::transform(
                r,
                [table](const auto& el) {
                    std::optional<std::string> store;
                    std::string_view           sv;
                    if constexpr (std::convertible_to<std::remove_cvref_t<decltype(el)>,
                                                      std::string_view>) {
                        sv = el;
                    } else if constexpr (std::convertible_to<std::remove_cvref_t<decltype(el)>,
                                                             std::string>) {
                        store = el;
                        sv = *store;
                    }
                    return fmt::format("{}{} {}",
                                       suffix(table.empty() || sv.contains('.'), table, "."),
                                       sv,
                                       eq == Eq::EQ ? "IS NULL" : "IS NOT NULL");
                }),
            op == Logical::AND ? " AND " : " OR "));
}

inline auto meta_prop_names(const QMetaObject& meta) {
    return std::views::transform(std::views::iota(0, meta.propertyCount()), [&meta](int i) {
        return std::string_view(meta.property(i).name());
    });
}

template<typename T, std::ranges::range R>
inline auto range_to(const R& r) {
    return T { r.begin(), r.end() };
}

template<Logical op, Eq eq>
inline auto null(const QMetaObject& meta, std::string_view table = {}) -> std::string {
    return null<op, eq>(meta_prop_names(meta), table);
}

} // namespace qcm::db