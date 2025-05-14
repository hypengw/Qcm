#ifdef __linux__
module;
#    include <cstdio>
#    include <pthread.h>
#    include <unistd.h>
module platform;

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
    auto term = helper::get_env_var("TERM");
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