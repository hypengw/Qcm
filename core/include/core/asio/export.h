#pragma once
#include "core/macro.h"

#if defined(QCM_ASIO_EXPORT)
#    define QCM_ASIO_API C_DECL_EXPORT
#else
#    define QCM_ASIO_API C_DECL_IMPORT
#endif