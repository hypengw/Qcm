#include "qcm_interface/notifier.h"
#include "qcm_interface/global.h"

namespace qcm
{

auto Notifier::instance() -> Notifier* { return Global::instance()->notifier(); };
Notifier::Notifier(QObject* parent): QObject(parent) {}
Notifier::~Notifier() {}

Notifier* Notifier::create(QQmlEngine*, QJSEngine*) {
    auto act = instance();
    // not delete on qml
    QJSEngine::setObjectOwnership(act, QJSEngine::CppOwnership);
    return act;
}
} // namespace qcm
