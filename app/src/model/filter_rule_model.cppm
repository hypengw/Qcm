module;
#include "QExtra/macro_qt.hpp"

#ifdef Q_MOC_RUN
#include "Qcm/model/filter_rule_model.moc"
#endif

export module qcm:model.filter_rule;
export import :msg;
export import qextra;

export namespace qcm
{

class FilterRuleModel : public kstore::QGadgetListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(bool dirty READ dirty NOTIFY dirtyChanged FINAL)
    Q_PROPERTY(QList<qcm::msg::filter::FilterLogic> filterLogics READ filterLogics WRITE
                   setFilterLogics NOTIFY filterLogicsChanged FINAL)
public:
    FilterRuleModel(kstore::QListInterface* list, QObject* = nullptr);
    ~FilterRuleModel();

    Q_SIGNAL void apply();
    Q_SIGNAL void reset();

    Q_INVOKABLE QString toJson() const;
    Q_SLOT void         fromJson(const QString&);

    auto toJsonDocument() const -> QJsonDocument;
    void fromJsonDocument(const QJsonDocument&);

    auto          dirty() const noexcept -> bool { return m_dirty; }
    void          setDirty(bool v);
    Q_SIGNAL void dirtyChanged();

    auto          filterLogics() const noexcept -> const QList<msg::filter::FilterLogic>& {
        return m_filter_logics;
    }
    void          setFilterLogics(const QList<msg::filter::FilterLogic>&);
    Q_SIGNAL void filterLogicsChanged();

    // section / grouping helpers
    Q_INVOKABLE int  newGroupId() const;
    Q_INVOKABLE int  countInGroup(int group) const;
    Q_INVOKABLE int  rowIndexInGroup(int row) const;
    Q_INVOKABLE int  rowCountInGroupOf(int row) const;
    Q_INVOKABLE int  findInsertPosition(int group) const;
    Q_INVOKABLE int  sectionIndexForGroup(int group) const;
    Q_INVOKABLE int  findLogicAt(int section_index) const;
    Q_INVOKABLE int  logicOpAt(int section_index) const;  // -1 if none
    Q_INVOKABLE void setLogicOpAt(int section_index, int op);
    Q_INVOKABLE void appendRuleInGroup(int group);
    Q_INVOKABLE void appendNewGroup();
    Q_INVOKABLE void deleteGroup(int group);
    Q_INVOKABLE void sortByGroup();

private:
    virtual void fromVariantlist(const QVariantList& v) = 0;

    int  roleForName_(const QByteArray& name) const;
    int  groupOf_(const QVariant& v) const;
    auto orderedGroups_() const -> QList<int>;

    Q_SLOT void                     markDirty();
    bool                            m_dirty;
    QList<msg::filter::FilterLogic> m_filter_logics;
};

class AlbumFilterRuleModel
    : public FilterRuleModel,
      public kstore::QMetaListModelCRTP<msg::filter::AlbumFilter, AlbumFilterRuleModel,
                                        kstore::ListStoreType::Vector> {
    Q_OBJECT
    QML_ELEMENT
public:
    AlbumFilterRuleModel(QObject* = nullptr);
    ~AlbumFilterRuleModel();

    void fromVariantlist(const QVariantList& v) override;
};

class ArtistFilterRuleModel
    : public FilterRuleModel,
      public kstore::QMetaListModelCRTP<msg::filter::ArtistFilter, ArtistFilterRuleModel,
                                        kstore::ListStoreType::Vector> {
    Q_OBJECT
    QML_ELEMENT
public:
    ArtistFilterRuleModel(QObject* = nullptr);
    ~ArtistFilterRuleModel();

    void fromVariantlist(const QVariantList& v) override;
};

class MixFilterRuleModel
    : public FilterRuleModel,
      public kstore::QMetaListModelCRTP<msg::filter::MixFilter, MixFilterRuleModel,
                                        kstore::ListStoreType::Vector> {
    Q_OBJECT
    QML_ELEMENT
public:
    MixFilterRuleModel(QObject* = nullptr);
    ~MixFilterRuleModel();

    void fromVariantlist(const QVariantList& v) override;
};

} // namespace qcm

