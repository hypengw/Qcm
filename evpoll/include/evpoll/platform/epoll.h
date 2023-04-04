#pragma once

#include <sys/epoll.h>
#include <sys/timerfd.h>

#include <optional>
#include <utility>

#include "core/log.h"

#include "evpoll/common.h"

namespace evpoll
{

constexpr auto ReadFlags { EPOLLIN | EPOLLRDHUP | EPOLLPRI };
constexpr auto WriteFlags { EPOLLOUT };
constexpr auto ErrorFlags { EPOLLERR };

struct Events {
    std::array<epoll_event, MaxPollEvent> list;
    usize                                 len;

    std::span<Event> map_event(std::span<Event> evs) {
        std::transform(list.begin(), list.begin() + len, evs.begin(), [](epoll_event& ev) {
            Event e;
            e.key = ev.data.u64;
            e.types.set(EventType::IN, ev.events & ReadFlags);
            e.types.set(EventType::OUT, ev.events & WriteFlags);
            e.types.set(EventType::ERR, ev.events & ErrorFlags);
            return e;
        });
        return evs.subspan(0, len);
    }
};

struct Error {
    int              code;
    std::string_view message() { return strerror(code); }
};

class Poller : public PollerBase<Poller, Events, Error>, NoCopy {
    friend class PollerBase<Poller, Events, Error>;

public:
    ~Poller() {
        std::array fds { m_epoll_fd, m_timer_fd };
        for (auto fd : fds)
            if (fd) close(fd);
    }
    Poller(Poller&& o) noexcept
        : m_epoll_fd(std::exchange(o.m_epoll_fd, 0)), m_timer_fd(std::exchange(o.m_timer_fd, 0)) {}
    Poller& operator=(Poller&& o) {
        m_epoll_fd = std::exchange(o.m_epoll_fd, 0);
        m_timer_fd = std::exchange(o.m_timer_fd, 0);
        return *this;
    }
    static Poller create();

    auto add(int fd, Event ev, Mode mode) { return ctl(EPOLL_CTL_ADD, fd, ev, mode); }
    auto del(int fd) { return ctl(EPOLL_CTL_DEL, fd, std::nullopt, std::nullopt); }
    auto mod(int fd, Event ev, Mode mode) { return ctl(EPOLL_CTL_MOD, fd, ev, mode); }

private:
    Poller() = default;

    std::optional<Error> wait_impl(Events& evs, std::optional<std::chrono::nanoseconds> timeout) {
        using namespace std::chrono;

        if (timeout) {
            auto       sec  = duration_cast<seconds>(timeout.value());
            auto       nano = timeout.value() - sec;
            itimerspec spec {};
            spec.it_interval      = { 0, 0 };
            spec.it_value.tv_sec  = sec.count();
            spec.it_value.tv_nsec = nano.count();

            timerfd_settime(m_timer_fd, 0, &spec, NULL);

            mod(m_timer_fd, Event::read(PollerKey), Mode::LTOneShot);
        }

        int len = epoll_wait(m_epoll_fd, evs.list.data(), evs.list.size(), -1);
        if (len > 0) {
            evs.len = (usize)len;
            return std::nullopt;
        } else {
            return Error { errno };
        }
    }

    std::optional<Error> ctl(int op, int fd, std::optional<Event> ev, std::optional<Mode> mode) {
        u32 flags = 0;
        switch (mode.value_or(Mode::LevelTriggered)) {
        case Mode::ETOneShot: flags |= EPOLLET | EPOLLONESHOT; break;
        case Mode::LTOneShot: flags |= EPOLLONESHOT; break;
        case Mode::EdgeTriggered: flags |= EPOLLET; break;
        case Mode::LevelTriggered:
        default: break;
        };
        if (ev) {
            if (ev->types[EventType::IN]) flags |= ReadFlags;
            if (ev->types[EventType::OUT]) flags |= WriteFlags;
        }
        epoll_event ep_ev {
            .events = flags,
            .data   = {},
        };

        if (epoll_ctl(m_epoll_fd, op, fd, &ep_ev) == -1) return Error { errno };
        return std::nullopt;
    }

private:
    int m_epoll_fd;
    int m_timer_fd;
};
inline Poller Poller::create() {
    auto epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    _assert_(epoll_fd != -1);

    auto timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    _assert_(timer_fd != -1);

    Poller p;
    p.m_epoll_fd = epoll_fd;
    p.m_timer_fd = timer_fd;

    p.add(timer_fd, Event::none(PollerKey), Mode::LTOneShot);
    return p;
}

} // namespace evpoll
