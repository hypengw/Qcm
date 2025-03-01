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
} // namespace plt
#endif