module;
#include "QExtra/macro_qt.hpp"

#ifdef Q_MOC_RUN
#    include "Qcm/model/id_queue.moc"
#endif

export module qcm:model.id_queue;
export import :model.item_id;
export import :msg.backend;

namespace qcm::model
{

export class IdQueue : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(qint32 currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY
                   currentIndexChanged BINDABLE bindableCurrentIndex FINAL)
    Q_PROPERTY(qcm::model::ItemId currentId READ currentId NOTIFY currentIdChanged FINAL)
    Q_PROPERTY(Options options READ options CONSTANT FINAL)
    Q_PROPERTY(QString name READ name CONSTANT FINAL)
public:
    IdQueue(QObject* parent = nullptr);
    ~IdQueue();

    enum Option
    {
        NoOptions         = 0,
        SupportShuffle    = 1,
        SupportLoop       = 1 << 1,
        SupportPrev       = 1 << 2,
        SupportJump       = 1 << 3,
        SupportUserRemove = 1 << 4,
        Dynamic           = 1 << 5,
    };
    Q_DECLARE_FLAGS(Options, Option)

    struct Item {
        ItemId   id;
        QVariant extra;
    };

    auto rowCount(const QModelIndex& parent = QModelIndex()) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;

    auto          options() const -> Options;
    auto          currentId() const -> ItemId;
    Q_SIGNAL void currentIdChanged();
    auto          currentExtra() const -> std::optional<QVariant>;
    auto          currentOrFirstExtra() const -> std::optional<QVariant>;
    auto          currentIndex() const -> qint32;
    Q_SLOT void   setCurrentIndex(qint32 idx);
    void          setCurrentIndex(const ItemId&);
    auto          bindableCurrentIndex() -> QBindable<qint32>;
    Q_SIGNAL void currentIndexChanged(qint32 idx);
    Q_SIGNAL void requestNext();
    auto          contains(const ItemId&) const -> bool;
    auto          insert(qint32 pos, std::span<const ItemId>) -> int;
    auto          insert(qint32 pos, std::span<const Item>) -> int;
    void          remove(const ItemId&);
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
                  const QModelIndex& destinationParent, int destinationChild) override;

    Q_INVOKABLE bool move(qint32 src, qint32 dst, qint32 count = 1);

    auto name() const -> const QString&;

protected:
    void setOptions(Options);
    void setName(QStringView);

private:
    Q_OBJECT_BINDABLE_PROPERTY(IdQueue, int, m_current_index, &IdQueue::currentIndexChanged)

    std::vector<Item>         m_queue;
    std::unordered_set<usize> m_set;
    Options                      m_opts;
    QString                      m_name;
};

export class DynamicIdQueue : public IdQueue {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("DynamicIdQueue is owned by PlayQueue")
    Q_PROPERTY(QList<qcm::msg::model::QueueConfigField> configuration READ configuration NOTIFY
                   configurationChanged FINAL)
    Q_PROPERTY(bool exhausted READ exhausted NOTIFY sessionChanged FINAL)
    Q_PROPERTY(bool configuring READ configuring NOTIFY configuringChanged FINAL)
    Q_PROPERTY(bool canRestart READ canRestart NOTIFY sessionChanged FINAL)
    Q_PROPERTY(QString restartLabel READ restartLabel NOTIFY sessionChanged FINAL)
    Q_PROPERTY(QString description READ description NOTIFY sessionChanged FINAL)
public:
    DynamicIdQueue(qint64 queue_id, const QString& name,
                   const QList<msg::model::QueueConfigField>& configuration, bool can_restart,
                   const QString& restart_label, QObject* parent = nullptr);
    ~DynamicIdQueue();
    auto             queueId() const -> qint64;
    auto             configuration() const -> const QList<msg::model::QueueConfigField>&;
    auto             configuring() const -> bool;
    auto             exhausted() const -> bool;
    auto             canRestart() const -> bool;
    auto             restartLabel() const -> QString;
    auto             description() const -> QString;
    Q_INVOKABLE void ensureStarted();
    Q_INVOKABLE void configure(const QString& key, const QString& value);
    Q_INVOKABLE void restart();
    Q_SIGNAL void    configurationChanged();
    Q_SIGNAL void    configuringChanged();
    Q_SIGNAL void    sessionChanged();
    Q_SIGNAL void    restarting();
    Q_SIGNAL void    restarted();

private:
    void open(msg::QueueOpenReasonGadget::QueueOpenReason reason,
              const QHash<QString, QString>&              configuration);
    void close(const QString& session_id);
    void request_next();
    void on_request_next();
    void on_query_finished();
    void on_open_finished();
    void on_current_index_changed(qint32 idx);
    auto values() const -> QHash<QString, QString>;

    qint64                              m_queue_id;
    QObject*                            m_query;
    QObject*                            m_open_query;
    bool                                m_auto_removing { false };
    bool                                m_configuring { false };
    bool                                m_can_restart;
    bool                                m_session_expired { false };
    bool                                m_suppress_prefetch { false };
    QString                             m_restart_label;
    msg::model::QueueSession            m_session;
    QList<msg::model::QueueConfigField> m_configuration;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(IdQueue::Options)

} // namespace qcm::model
