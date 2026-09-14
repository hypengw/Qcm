module;
#include "core/macro.h"

export module qcm.player:player;
import qcm.core;
import rstd.cppstd;
import :notify;

export namespace player
{

class Player {
public:
    class Private;
    Player(std::string_view                             name, Notifier,
           rstd::ref<rstd::dyn<rstd::alloc::Allocator>> allocator =
               ::alloc::allocator_ref(::alloc::GLOBAL));
    ~Player();

    auto process_actions() -> rstd::async::coro<void>;
    void close();

    void play();
    void pause();
    void stop();
    void seek(i32);

    void set_source(std::string_view);

    auto volume() const -> float;
    void set_volume(float);

    auto fade_time() const -> u32;
    void set_fade_time(u32);

private:
    C_DECLARE_PRIVATE(Player, m_d)
};

} // namespace player
