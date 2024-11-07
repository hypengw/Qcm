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
    Not = 0,
    EQ
};
using enum Logical;
using enum Eq;

constexpr auto suffix(std::string_view in, std::string_view suffix_) -> std::string {
    return in.empty() ? std::string(in) : fmt::format("{}{}", in, suffix_);
}

template<Logical op, Eq eq, std::ranges::range R>
auto null(const R& r, std::string_view table = {}) -> std::string {
    return fmt::format("{}",
                       fmt::join(std::views::transform(r,
                                                       [table](const auto& el) {
                                                           return fmt::format("{}{} {}",
                                                                              suffix(table, "."),
                                                                              el,
                                                                              eq == Eq::EQ
                                                                                  ? "IS NULL"
                                                                                  : "IS NOT NULL");
                                                       }),
                                 op == Logical::AND ? " AND " : " OR "));
}

inline auto meta_prop_names(const QMetaObject& meta) {
    return std::views::transform(std::views::iota(0, meta.propertyCount()), [&meta](int i) {
        return meta.property(i).name();
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