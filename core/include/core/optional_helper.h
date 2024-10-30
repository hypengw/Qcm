#pragma once

#include <optional>
#include "core/core.h"

namespace helper
{
template<typename T>
concept is_optional = ycore::is_specialization_of_v<std::decay_t<T>, std::optional>;

template<typename T>
auto to_optional(T* pointer) -> std::optional<std::reference_wrapper<T>> {
    if (pointer) {
        return { *pointer };
    } else {
        return std::nullopt;
    }
}

template<typename T, typename TKey>
    requires ycore::MapConcept<std::remove_cvref_t<T>>
auto to_optional(T& container, const TKey& key) -> std::optional<typename T::mapped_type> {
    if (auto it = container.find(key); it != container.end()) return it->second;
    return std::nullopt;
}

} // namespace helper