#pragma once
#include <functional>
#include "core/core.h"

namespace media_cache
{
struct Fallbacks {
    // last write fragment
    std::function<void(usize begin, usize end, usize totle)> fragment;
};
} // namespace media_cache