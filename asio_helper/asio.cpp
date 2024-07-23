#include "core/core.h"

#ifdef __linux__
#define ASIO_DECL C_DECL_EXPORT
#endif

#undef ASIO_DISABLE_VISIBILITY

#include <asio/impl/src.hpp>