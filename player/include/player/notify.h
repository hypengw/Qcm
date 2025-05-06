#pragma once

#include <variant>
#include "core/sender.h"
#include "core/helper.h"

namespace player
{
enum class PlayState
{
    Playing = 0,
    Paused,
    Stopped
};

namespace notify
{
template<typename T>
struct base {
    T value {};
};

struct position : base<i64> {};
struct duration : base<i64> {};
struct playstate : base<PlayState> {};
struct busy : base<bool> {};
struct cache {
    float begin;
    float end;
};

using info = std::variant<position, duration, playstate, busy, cache>;
} // namespace notify
//
using Notifier = qcm::Sender<notify::info>;

} // namespace player
