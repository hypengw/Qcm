module;
#include "Qcm/notifier.moc.h"
module qcm;
import :notifier;

namespace qcm
{

auto Notifier::instance() -> Notifier* {
    static auto the =
        GlobalStatic::instance()->add<Notifier>("notifier", new Notifier(nullptr), [](Notifier* p) {
            delete p;
        });
    return the;
};

Notifier::Notifier(QObject* parent): QObject(parent) {}
Notifier::~Notifier() {}

Notifier* Notifier::create(QQmlEngine*, QJSEngine*) {
    auto act = instance();
    // not delete on qml
    QJSEngine::setObjectOwnership(act, QJSEngine::CppOwnership);
    return act;
}
} // namespace qcm

#include "Qcm/notifier.moc.cpp"