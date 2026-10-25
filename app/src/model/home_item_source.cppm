module;
#include "qml_material/model/item_source.hpp"
#ifdef Q_MOC_RUN
#    include "Qcm/model/home_item_source.moc"
#endif
export module qcm:model.home_item_source;
export import :model.list_models;

export namespace qcm::model
{
class HomeCardData {
    Q_GADGET
    QML_VALUE_TYPE(home_card_data)
    Q_PROPERTY(QString key MEMBER key)
    Q_PROPERTY(qcm::model::ItemId itemId MEMBER itemId)
    Q_PROPERTY(qcm::model::ItemId coverId MEMBER coverId)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString title READ title)
    Q_PROPERTY(QString detail MEMBER detail)
    Q_PROPERTY(QString description READ description)
    Q_PROPERTY(QString badge MEMBER badge)
    Q_PROPERTY(QString entryKey MEMBER entryKey)
    Q_PROPERTY(quint32 trackCount MEMBER trackCount)
    Q_PROPERTY(QList<qcm::model::Artist> artists MEMBER artists)
    Q_PROPERTY(qcm::model::ForYouCard card MEMBER card)
    Q_PROPERTY(qcm::model::RadioQueue radio MEMBER radio)
    Q_PROPERTY(int style MEMBER style)
    Q_PROPERTY(int coverKind MEMBER coverKind)
    Q_PROPERTY(int playAction MEMBER playAction)
public:
    QString       key;
    ItemId        itemId, coverId;
    QString       name, detail, badge, entryKey;
    quint32       trackCount {};
    QList<Artist> artists;
    ForYouCard    card;
    RadioQueue    radio;
    int           style {}, coverKind {}, playAction {};
    QString       title() const { return name; }
    QString       description() const { return detail; }
    bool          operator==(const HomeCardData&) const = default;
};

class HomeItemSource : public qml_material::ItemSource {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QAbstractItemModel* model READ model WRITE setModel NOTIFY modelChanged FINAL)
public:
    using ItemSource::ItemSource;
    QAbstractItemModel* model() const { return m_model; }
    void                setModel(QAbstractItemModel*);
    Q_SIGNAL void       modelChanged();

private:
    void                           rebuild(int first = -1, int last = -1);
    QPointer<QAbstractItemModel>   m_model;
    QList<QMetaObject::Connection> m_connections;
};
} // namespace qcm::model
