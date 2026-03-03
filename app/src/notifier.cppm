module;
#include "Qcm/macro_qt.hpp"
#ifdef Q_MOC_RUN
#include "Qcm/notifier.moc"
#endif
export module qcm:notifier;
export import :global;
export import :model.item_id;
export import :qml.enums;

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