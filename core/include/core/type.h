#pragma once

#include <source_location>

import qcm.core;
#include "core/helper.h"
#include "core/array_helper.h"

struct YCORE_CLASS_REF {};

namespace ycore
{

namespace detail
{

template<typename T>
constexpr auto function_name() noexcept -> std::string_view {
    return std::source_location::current().function_name();
}

struct NameInfo {
    usize function_name_size;
    usize begin;
};

constexpr std::string_view YCORE_CLASS_REF_SV { "YCORE_CLASS_REF" };

constexpr NameInfo class_name_info {
    .function_name_size = function_name<YCORE_CLASS_REF>().size() - YCORE_CLASS_REF_SV.size(),
    .begin              = function_name<YCORE_CLASS_REF>().find(YCORE_CLASS_REF_SV)
};

template<typename T>
constexpr auto type_name_info() noexcept -> NameInfo {
    if constexpr (std::is_class_v<T>) {
        return class_name_info;
    } else {
        static_assert(false);
        return {};
    }
}

template<typename T>
constexpr auto type_qualifier_size() noexcept -> usize {
    if constexpr (std::is_const_v<T>) {
        return type_qualifier_size<std::remove_const_t<T>>() + 1;
    } else if constexpr (std::is_volatile_v<T>) {
        return type_qualifier_size<std::remove_volatile_t<T>>() + 1;
    } else if constexpr (std::is_reference_v<T>) {
        return type_qualifier_size<std::remove_reference_t<T>>() + 1;
    } else if constexpr (std::is_pointer_v<T>) {
        return type_qualifier_size<std::remove_pointer_t<T>>() + 1;
    } else {
        return 1;
    }
};

} // namespace detail

template<typename T, usize N = detail::type_qualifier_size<T>()>
constexpr auto type_name_list() noexcept -> std::array<std::string_view, N> {
    if constexpr (std::is_const_v<T>) {
        return concat(type_name_list<std::remove_const_t<T>>(), "const"sv);
    } else if constexpr (std::is_volatile_v<T>) {
        return concat(type_name_list<std::remove_volatile_t<T>>(), "volatile"sv);
    } else if constexpr (std::is_reference_v<T>) {
        return concat(type_name_list<std::remove_reference_t<T>>(), "&"sv);
    } else if constexpr (std::is_pointer_v<T>) {
        return concat(type_name_list<std::remove_pointer_t<T>>(), "*"sv);
    } else {
        constexpr auto name_info = detail::type_name_info<T>();
        constexpr auto func_name = detail::function_name<T>();
        constexpr auto t_name =
            func_name.substr(name_info.begin, func_name.size() - name_info.function_name_size);
        return { t_name };
    }
}

constexpr auto test_ { type_name_list<std::string_view&>() };

template<typename T>
constexpr auto type_name() noexcept -> std::string_view {
    constexpr auto name_list = type_name_list<T>();
    if constexpr (name_list.size() == 1) {
        return name_list.front();
    } else {
        return ([&name_list] {
            static auto arr_name { join<join_size(name_list, " ")>(name_list, " ") };
            return std::string_view { arr_name.data(), arr_name.size() };
        })();
    }
};

} // namespace ycore