#pragma once

#ifdef __linux__

extern "C" {
#    include <pthread.h>
}

#endif

namespace qcm
{

inline void set_thread_name(const char* name) {
#ifdef __linux__
    pthread_setname_np(pthread_self(), name);
#endif
}

} // namespace qcm