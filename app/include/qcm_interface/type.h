#pragma once

#include <QString>
#include <QVariant>

import qcm.core;
#include "core/helper.h"
#include "core/macro.h"
#include "core/qstr_helper.h"

template<typename T>
    requires ycore::extra_cvt<QVariant, T>
struct Convert<QVariant, T> {
    static void from(QVariant& out, const T& in) { out = QVariant::fromValue(in); }
};

namespace qcm
{
template<typename TApi>
struct api_traits {
    using api_type = TApi;
    using out_type = typename TApi::out_type;

    template<typename T>
    static void handle_output(const api_type&, T&);
};
} // namespace qcm

// namespace qcm