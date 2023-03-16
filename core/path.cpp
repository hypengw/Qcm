#include "core/path.h"
#include "core/log.h"

#include <cstdlib>

using path = std::filesystem::path;

namespace xdg
{
constexpr std::string_view AppName { "Qcm" };

path home() {
    static path home_path = []() {
        char* env_home = std::getenv("HOME");
        _assert_msg_(env_home != NULL, "no HOME env");
        return env_home;
    }();
    return home_path;
}

namespace config
{
path home() {
    static path home_path = []() {
        char* env_path = std::getenv("XDG_CONFIG_HOME");
        return env_path != NULL ? env_path : xdg::home() / ".config";
    }();
    return home_path;
}
} // namespace config

namespace data
{
path home() {
    static path home_path = []() {
        char* env_path = std::getenv("XDG_DATA_HOME");
        return env_path != NULL ? env_path : xdg::home() / ".data";
    }();
    return home_path;
}
} // namespace data

namespace cache
{
path home() {
    static path home_path = []() {
        char* env_path = std::getenv("XDG_CACHE_HOME");
        return env_path != NULL ? env_path : xdg::home() / ".cache";
    }();
    return home_path;
}
} // namespace cache

} // namespace xdg

std::filesystem::path qcm::config_path() { return xdg::config::home() / xdg::AppName; }

std::filesystem::path qcm::data_path() { return xdg::data::home() / xdg::AppName; }

std::filesystem::path qcm::cache_path() { return xdg::cache::home() / xdg::AppName; }

bool qcm::init_path(std::span<const std::filesystem::path> pathes) {
    for (auto& p : pathes) {
        std::error_code ec;
        std::filesystem::create_directories(p, ec);
        _assert_msg_(! ec, "path: {}, info: {}({})", p.native(), ec.message(), ec.value())
    }
    return true;
}
