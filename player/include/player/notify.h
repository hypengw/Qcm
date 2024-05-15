#pragma once

#include "core/sender.h"
#include "core/variant_helper.h"

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

using info = std::variant<position, duration, playstate, busy>;
} // namespace notify
//
using Notifier = qcm::Sender<notify::info>;

} // namespace player
