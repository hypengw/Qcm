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

struct position : public base<i64> {};
struct duration : public base<i64> {};
struct playstate : public base<PlayState> {};

using info = std::variant<position, duration, playstate>;
} // namespace notify
//
using Notifier = qcm::Sender<notify::info>;

} // namespace player
