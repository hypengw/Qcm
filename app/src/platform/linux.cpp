module;
#ifdef __linux__
#    include <pthread.h>
#endif
module platform;

#ifdef __linux__
namespace plt
{
void set_thread_name(const char* name) { pthread_setname_np(pthread_self(), name); }

} // namespace plt
#endif
