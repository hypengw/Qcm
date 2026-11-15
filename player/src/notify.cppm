module;
#include <rstd/enum.hpp>

export module qcm.player:notify;
export import rstd;

using namespace rstd::prelude;
using rstd::sync::Arc;

export namespace player
{
enum class PlayState
{
    Playing = 0,
    Paused,
    Stopped
};

namespace notify
{
class info final {
    RSTD_ENUM_DEFAULT(info, (position, 0), (position, (rstd::int64_t value;)),
                      (duration, (rstd::int64_t value;)), (playstate, (PlayState value;)),
                      (busy, (bool value;)), (cache, (float begin; float end;)),
                      (seekable, (bool value;)), (ended))
};
} // namespace notify

namespace detail
{
struct NotifySink {
    template<class Self, class = void>
    struct Api {
        using Trait = NotifySink;
        auto send(notify::info value) const -> bool {
            return rstd::trait_call<0>(this, rstd::move(value));
        }
        auto advance_epoch() const -> rstd::uint64_t { return rstd::trait_call<1>(this); }
        void error(rstd::uint64_t id, ref<str> text) const { rstd::trait_call<2>(this, id, text); }
        void seeked(rstd::uint64_t id, rstd::int64_t position) const {
            rstd::trait_call<3>(this, id, position);
        }
        void command_consumed() const { rstd::trait_call<4>(this); }
    };
    template<class T>
    using Funcs =
        rstd::TraitFuncs<&T::send, &T::advance_epoch, &T::error, &T::seeked, &T::command_consumed>;
};
} // namespace detail

class Notifier {
public:
    Notifier() = default;
    explicit Notifier(Arc<dyn<detail::NotifySink>> sink): m_sink(Some(rstd::move(sink))) {}

    bool send(notify::info info) const {
        return m_sink.is_some() && (*m_sink)->send(rstd::move(info));
    }
    bool try_send(notify::info info) const { return send(rstd::move(info)); }
    auto advance_epoch() const -> rstd::uint64_t {
        return m_sink.is_some() ? (*m_sink)->advance_epoch() : 0;
    }
    void error(rstd::uint64_t id, ref<str> text) const {
        if (m_sink.is_some()) (*m_sink)->error(id, text);
    }
    void seeked(rstd::uint64_t id, rstd::int64_t position) const {
        if (m_sink.is_some()) (*m_sink)->seeked(id, position);
    }
    void command_consumed() const {
        if (m_sink.is_some()) (*m_sink)->command_consumed();
    }

private:
    Option<Arc<dyn<detail::NotifySink>>> m_sink;
};

} // namespace player
