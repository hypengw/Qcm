module;
#include "QExtra/macro_qt.hpp"
#ifdef Q_MOC_RUN
#    include "Qcm/model/id_queue.moc"
#endif

export module qcm:model.id_queue;
export import :model.item_id;
export import :msg.backend;
import qextra;

namespace qcm::model
{
export class DynamicIdQueue : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("DynamicIdQueue is owned by PlayQueue")
    Q_PROPERTY(qcm::model::ItemId currentId READ currentId NOTIFY sessionChanged FINAL)
    Q_PROPERTY(QString name READ name NOTIFY sessionChanged FINAL)
    Q_PROPERTY(QList<qcm::msg::model::QueueConfigField> configuration READ configuration NOTIFY
                   configurationChanged FINAL)
    Q_PROPERTY(bool exhausted READ exhausted NOTIFY sessionChanged FINAL)
    Q_PROPERTY(bool configuring READ configuring NOTIFY configuringChanged FINAL)
    Q_PROPERTY(bool canRestart READ canRestart NOTIFY sessionChanged FINAL)
    Q_PROPERTY(QString restartLabel READ restartLabel NOTIFY sessionChanged FINAL)
    Q_PROPERTY(QString description READ description NOTIFY sessionChanged FINAL)

public:
    DynamicIdQueue(const msg::id::RadioQueueId&, QObject* parent = nullptr);
    void updateDefinition(const QString&, const QList<msg::model::QueueConfigField>&,
                          bool can_restart, const QString& restart_label);
    void applyState(const msg::PlayerQueueMsg&);
    void disconnectQueue();
    auto queueId() const -> msg::id::RadioQueueId;
    auto currentId() const -> ItemId;
    auto name() const -> QString;
    auto configuration() const -> const QList<msg::model::QueueConfigField>&;
    auto configuring() const -> bool;
    auto exhausted() const -> bool;
    auto canRestart() const -> bool;
    auto restartLabel() const -> QString;
    auto description() const -> QString;

    void             activate();
    Q_INVOKABLE void configure(const QString& key, const QString& value);
    Q_INVOKABLE void restart();
    Q_SIGNAL void    configurationChanged();
    Q_SIGNAL void    configuringChanged();
    Q_SIGNAL void    sessionChanged();

private:
    void request(msg::QueueOpenReasonGadget::QueueOpenReason, const QHash<QString, QString>&);
    auto values() const -> QHash<QString, QString>;

    msg::id::RadioQueueId               m_queue_id;
    QString                             m_name;
    QList<msg::model::QueueConfigField> m_configuration;
    QHash<QString, QString>             m_values;
    ItemId                              m_current_id;
    bool                                m_has_values {};
    bool                                m_exhausted {};
    bool                                m_configuring {};
    bool                                m_can_restart {};
    quint64                             m_request_serial {};
    QString                             m_restart_label;
    QString                             m_description;
};
} // namespace qcm::model
