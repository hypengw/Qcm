#ifdef __linux__
module;
#    include <cstdio>
#    include <pthread.h>
#    include <unistd.h>
#    include <cstdlib>
#    include <string_view>
#    include <optional>
module platform;

namespace
{
auto get_env_var(std::string_view var_name) -> std::optional<std::string_view> {
    if (const char* value = std::getenv(var_name.data())) {
        return std::string_view(value);
    }
    return std::nullopt;
}
} // namespace

namespace plt
{
void set_thread_name(const char* name) { pthread_setname_np(pthread_self(), name); }

auto is_terminal() -> bool { return isatty(fileno(stdout)); }

auto support_color() -> bool {
    // Check if the standard output is a terminal
    if (! plt::is_terminal()) {
        return false;
    }

    // Get the TERM environment variable
    auto term = get_env_var("TERM");
    if (! term) {
        return false;
    }

    // Check if the TERM value contains keywords indicating color support
    return term->find("color") != std::string_view::npos ||
           term->find("xterm") != std::string_view::npos ||
           term->find("screen") != std::string_view::npos ||
           term->find("tmux") != std::string_view::npos;
}
} // namespace plt
#endif