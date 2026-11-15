#pragma once
#include "Qcm/core/macro.h"

#define GADGET_PROPERTY(type, prop, var, val) \
    Q_PROPERTY(type prop MEMBER var FINAL)    \
    type var { val };

#define GADGET_PROPERTY_DEF(type, prop, var) GADGET_PROPERTY(type, prop, var, )

#if defined(qApp)
#    undef qApp
#endif
#define qApp (static_cast<QGuiApplication*>(QCoreApplication::instance()))
