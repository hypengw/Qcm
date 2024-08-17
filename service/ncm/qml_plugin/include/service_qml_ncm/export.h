#pragma once
#include "core/macro.h"

#if defined(SERVICE_QML_NCM_EXPORT)
#    define SERVICE_QML_NCM_API C_DECL_EXPORT
#else
#    define SERVICE_QML_NCM_API C_DECL_IMPORT
#endif