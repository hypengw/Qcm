#pragma once
#include "core/core.h"

#if defined(QCM_INTERFACE_EXPORT)
#    define QCM_INTERFACE_API C_DECL_EXPORT
#else
#    define QCM_INTERFACE_API C_DECL_IMPORT
#endif