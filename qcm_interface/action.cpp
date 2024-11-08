#include "qcm_interface/action.h"
#include "qcm_interface/global.h"
#include "qcm_interface/global_static.h"

namespace qcm
{

auto Action::instance() -> Action* {
    static auto the =
        GlobalStatic::instance()->add<Action>("action", new Action(nullptr), [](Action* p) {
            delete p;
        });
    return the;
};

Action::Action(QObject* parent): QObject(parent) {}
Action::~Action() {}

Action* Action::create(QQmlEngine*, QJSEngine*) {
    auto act = instance();
    // not delete on qml
    QJSEngine::setObjectOwnership(act, QJSEngine::CppOwnership);
    return act;
}
} // namespace qcm
