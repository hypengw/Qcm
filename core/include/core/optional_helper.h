#pragma once

#include <optional>
import qcm.core;

namespace helper
{
template<typename T>
concept is_optional = rstd::mtp::spec_of<std::decay_t<T>, std::optional>;

template<typename T>
auto to_optional(T* pointer) -> std::optional<std::reference_wrapper<T>> {
    if (pointer) {
        return { *pointer };
    } else {
        return std::nullopt;
    }
}
} // namespace helper