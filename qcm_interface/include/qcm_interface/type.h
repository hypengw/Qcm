#pragma once

#include <QString>
#include <QVariant>

#include "core/core.h"
#include "core/str_helper.h"
#include "core/qstr_helper.h"

template<typename T>
    requires ycore::extra_cvt<QVariant, T>
struct Convert<QVariant, T> {
    static void from(QVariant& out, const T& in) { out = QVariant::fromValue(in); }
};

// namespace qcm