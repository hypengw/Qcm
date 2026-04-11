module;
#include "Qcm/macro_qt.hpp"
#ifdef Q_MOC_RUN
#include "Qcm/qml/duration.moc"
#endif
export module qcm:qml.duration;
import qcm.core;
import qextra;

namespace qcm
{
export struct Duration {
    Q_GADGET
    QML_VALUE_TYPE(duration)
public:
    i64 value { 0 };
};
} // namespace qcm