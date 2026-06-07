#pragma once

#include <variant>
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

namespace detail
{
class NotifySink {
public:
    virtual ~NotifySink() = default;

    virtual bool send(notify::info) = 0;
    virtual auto advance_epoch() -> u64 = 0;
};
} // namespace detail

class Notifier {
public:
    Notifier() = default;
    explicit Notifier(rc<detail::NotifySink> sink): m_sink(std::move(sink)) {}

    bool send(notify::info info) const {
        return m_sink && m_sink->send(std::move(info));
    }
    bool try_send(notify::info info) const { return send(std::move(info)); }
    auto advance_epoch() const -> u64 { return m_sink ? m_sink->advance_epoch() : 0; }

private:
    rc<detail::NotifySink> m_sink;
};

} // namespace player
