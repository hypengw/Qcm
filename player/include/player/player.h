#pragma once

#include <map>
#include <filesystem>
#include <memory_resource>
#include <asio/experimental/concurrent_channel.hpp>
#include <asio/thread_pool.hpp>
#include <asio/strand.hpp>

#include "core/core.h"
#include "player/notify.h"
#include "player/metadata.h"

namespace player
{

auto get_metadata(const std::filesystem::path&) -> Metadata;

class Player {
public:
    using executor_type = asio::thread_pool::executor_type;

    class Private;
    Player(std::string_view name, Notifier, executor_type exc,
           std::pmr::memory_resource* mem = std::pmr::get_default_resource());
    ~Player();

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
