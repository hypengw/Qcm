#include "qcm_interface/action.h"
#include "qcm_interface/global.h"

namespace qcm
{

auto Action::instance() -> Action* { return Global::instance()->action(); };
Action::Action(QObject* parent): QObject(parent) {}
Action::~Action() {}

Action* Action::create(QQmlEngine*, QJSEngine*) {
    auto act = instance();
    // not delete on qml
    QJSEngine::setObjectOwnership(act, QJSEngine::CppOwnership);
    return act;
}
} // namespace qcm
