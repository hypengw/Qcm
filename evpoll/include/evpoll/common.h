#pragma once

#include <limits>
#include <span>
#include <chrono>

#include "core/core.h"
#include "core/expected_helper.h"
#include "core/bit_flags.h"

namespace evpoll
{
constexpr usize PollerKey { std::numeric_limits<usize>::max() };
constexpr int   MaxPollEvent { 1024 };

enum class EventType
{
    IN,
    OUT,
    ERR,
};

struct Event {
    using Types = BitFlags<EventType>;
    usize key;
    Types types;

    static Event none(usize key) { return Event { .key = key, .types = {} }; }
    static Event all(usize key) {
        return Event {
            .key   = key,
            .types = { EventType::IN, EventType::ERR },
        };
    }
    static Event read(usize key) {
        return Event {
            .key   = key,
            .types = { EventType::IN },
        };
    }
    static Event write(usize key) {
        return Event {
            .key   = key,
            .types = { EventType::OUT },
        };
    }
};

enum class Mode
{
    LevelTriggered,
    EdgeTriggered,
    LTOneShot,
    ETOneShot,
};

template<typename TPoller>
concept PollerCT = requires() { true; };
template<typename TEvents>
concept PollerEventsCT = requires(TEvents t, std::span<Event> es) {
                             { t.map_event(es) } -> std::convertible_to<std::span<Event>>;
                         };

template<typename IMPL, PollerEventsCT TEvents, typename TErr>
class PollerBase : public CRTP<IMPL> {
public:
    using error_type  = TErr;
    using events_type = TEvents;
    template<typename T>
    using Result = nstd::expected<T, error_type>;

    Result<std::span<Event>> wait(std::optional<std::chrono::nanoseconds> timeout)
        requires PollerCT<IMPL>
    {
        IMPL& self = this->crtp_impl();
        if (auto err = self.wait_impl(m_events, timeout); err) return nstd::unexpected(err.value());
        auto out = m_events.map_event(m_event_out);
        auto end = std::copy_if(out.begin(), out.end(), m_event_out.begin(), [](const Event& e) {
            return e.key != PollerKey;
        });
        return std::span { m_event_out.begin(), end };
    }

private:
    std::array<Event, MaxPollEvent> m_event_out;
    TEvents                         m_events;
};

} // namespace evpoll
