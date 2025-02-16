#include "platform/platform.h"
#include <pthread.h>

namespace plt
{
void set_thread_name(const char* name) { pthread_setname_np(pthread_self(), name); }
} // namespace plt