module;
#include "QExtra/macro_qt.hpp"
#include <QSet>
#include "Qcm/message/comment_reaction.hpp"
#ifdef Q_MOC_RUN
#    include "Qcm/query/comments_query.moc"
#endif
export module qcm:query.comments;
export import :query.query;
export import :model.store_item;

namespace qcm
{
export class CommentStore;
export class CommentView : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by CommentStore")
    Q_PROPERTY(qcm::msg::model::Comment value READ value NOTIFY changed FINAL)
    Q_PROPERTY(bool pending READ pending NOTIFY changed FINAL)
    Q_PROPERTY(bool uncertain READ uncertain NOTIFY changed FINAL)
    Q_PROPERTY(QString error READ error NOTIFY changed FINAL)
    Q_PROPERTY(QDateTime time READ time NOTIFY changed FINAL)
public:
    explicit CommentView(QObject* parent = nullptr): QObject(parent) {}
    msg::model::Comment value() const { return m_value; }
    bool                pending() const { return m_reaction.pending; }
    bool                uncertain() const { return m_reaction.uncertain; }
    QString             error() const { return m_error; }
    QDateTime           time() const {
        return m_value.hasTimeMs() ? QDateTime::fromMSecsSinceEpoch(m_value.timeMs()) : QDateTime();
    }
    Q_SIGNAL void changed();

private:
    friend class CommentStore;
    void                applyReaction();
    msg::model::Comment m_value;
    comments::Reaction  m_reaction;
    QString             m_error;
};

export class CommentStore : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool canLike READ canLike NOTIFY changed FINAL)
public:
    explicit CommentStore(QObject* parent = nullptr);
    ~CommentStore();
    bool        canLike() const { return m_can_like && ! m_scope.isEmpty(); }
    Q_INVOKABLE qcm::CommentView* view(const QString& id);
    Q_INVOKABLE void              setLiked(const QString& id, bool value);
    Q_INVOKABLE void              close() {
        m_closed = true;
        clear();
    }
    bool    closed() const { return m_closed; }
    bool    matchesTarget(model::ItemId id) const { return ! m_item_id.valid() || m_item_id == id; }
    void    clear();
    void    merge(const QList<msg::model::Comment>&, quint64 revision);
    void    bind(model::ItemId, const msg::model::CommentsResult&);
    QString scope() const { return m_scope; }
    quint64 tick() { return ++m_clock; }
    quint64 generation() const { return m_generation; }
    Q_SIGNAL void changed();
    Q_SIGNAL void invalidated();
    Q_SIGNAL void refreshRequested();
    Q_SIGNAL void reactionFailed(QString message);

private:
    void                         reconcileProvider();
    QHash<QString, CommentView*> m_entries;
    model::ItemId                m_item_id;
    QString                      m_scope;
    qint64                       m_provider_id {};
    quint64                      m_epoch {};
    quint64                      m_generation {};
    quint64                      m_clock {};
    bool                         m_can_like {};
    bool                         m_closed {};
    QAsyncScope                  m_jobs;
};

export class CommentListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
public:
    explicit CommentListModel(QObject* parent = nullptr): QAbstractListModel(parent) {}
    int count() const { return static_cast<int>(m_ids.size()); }
    int rowCount(const QModelIndex& parent = {}) const override {
        return parent.isValid() ? 0 : count();
    }
    QVariant               data(const QModelIndex&, int) const override;
    QHash<int, QByteArray> roleNames() const override {
        return { { Qt::UserRole, "commentView" } };
    }
    void          reset(CommentStore* store);
    void          append(const QList<msg::model::Comment>&);
    Q_SIGNAL void countChanged();

private:
    QPointer<CommentStore> m_store;
    QStringList            m_ids;
    QSet<QString>          m_seen;
};

export class CommentsQuery : public Query {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged FINAL)
    Q_PROPERTY(qcm::CommentStore* store READ store WRITE setStore NOTIFY storeChanged FINAL)
    Q_PROPERTY(qcm::CommentListModel* model READ model CONSTANT)
    Q_PROPERTY(QString parentId READ parentId WRITE setParentId NOTIFY parentIdChanged FINAL)
    Q_PROPERTY(int order READ order WRITE setOrder NOTIFY changed FINAL)
    Q_PROPERTY(bool supported READ supported NOTIFY changed FINAL)
    Q_PROPERTY(bool loaded READ loaded NOTIFY changed FINAL)
    Q_PROPERTY(bool hasMore READ hasMore NOTIFY changed FINAL)
    Q_PROPERTY(bool loadingMore READ loadingMore NOTIFY changed FINAL)
    Q_PROPERTY(bool canReply READ canReply NOTIFY changed FINAL)
    Q_PROPERTY(QVariant total READ total NOTIFY changed FINAL)
    Q_PROPERTY(QList<int> orders READ orders NOTIFY changed FINAL)
public:
    explicit CommentsQuery(QObject* parent = nullptr);
    model::ItemId     itemId() const { return m_item_id; }
    void              setItemId(model::ItemId);
    CommentStore*     store() const { return m_store; }
    void              setStore(CommentStore*);
    CommentListModel* model() { return &m_model; }
    QString           parentId() const { return m_parent_id; }
    void              setParentId(const QString&);
    int               order() const { return m_order; }
    void              setOrder(int);
    bool              supported() const { return m_supported; }
    bool              loaded() const { return m_loaded; }
    bool              hasMore() const { return m_has_more; }
    bool              loadingMore() const { return m_loading_more; }
    bool              canReply() const { return m_can_reply; }
    QVariant          total() const { return m_total; }
    QList<int>        orders() const { return m_orders; }
    void              reload() override;
    Q_INVOKABLE void  fetchMore();
    Q_INVOKABLE void  stop();
    Q_SIGNAL void     itemIdChanged();
    Q_SIGNAL void     parentIdChanged();
    Q_SIGNAL void     storeChanged();
    Q_SIGNAL void     changed();

private:
    void          reset();
    void          load(bool more);
    void          accept(const msg::model::CommentsResult&, bool more, quint64 read_revision);
    CommentStore* m_owned_store;
    QPointer<CommentStore> m_store;
    CommentListModel       m_model;
    model::ItemId          m_item_id;
    QString                m_parent_id;
    QString                m_token;
    QVariant               m_total;
    QList<int>             m_orders;
    int                    m_order {};
    bool                   m_supported {};
    bool                   m_loaded {};
    bool                   m_has_more {};
    bool                   m_loading_more {};
    bool                   m_can_reply {};
    bool                   m_stopped {};
};
} // namespace qcm
