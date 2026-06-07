#pragma once

#include <map>
#include <filesystem>
#include <memory_resource>
#include "core/macro.h"

#include "player/notify.h"
#include "player/metadata.h"

import qcm.core;

namespace player
{

auto get_metadata(const std::filesystem::path&) -> Metadata;

class Player {
public:
    class Private;
    Player(std::string_view name, Notifier,
           std::pmr::memory_resource* mem = std::pmr::get_default_resource());
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
