module;
#include "Qcm/action.moc.h"
module qcm;
import :action;

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


#include "Qcm/action.moc.cpp"

