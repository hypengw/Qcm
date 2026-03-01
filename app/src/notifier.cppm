module;


#include <QtQml/QQmlEngine>
#include <QtGui/QImage>

#include "Qcm/notifier.moc.h"


#ifdef Q_MOC_RUN
#include "Qcm/notifier.moc"
#endif

export module qcm.notifier;
export import qcm.global;
export import qcm.model.item_id;
export import qcm.qml.enums;

namespace qcm
{

export class Notifier : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    Notifier(QObject* parent);
    ~Notifier();
    static auto      instance() -> Notifier*;
    static Notifier* create(QQmlEngine*, QJSEngine*);
    // make qml prefer create
    Notifier() = delete;

Q_SIGNALS:
    void specialImageLoaded(const QString& name, QImage img);
    void mixCreated(const QString& name);
    void mixDeleted();
    void mixLinked();
};
} // namespace qcm

module :private;

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