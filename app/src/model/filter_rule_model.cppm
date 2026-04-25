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
    Q_PROPERTY(qcm::msg::filter::FilterLogicGadget::FilterLogic groupLogic READ groupLogic WRITE
                   setGroupLogic NOTIFY groupLogicChanged FINAL)
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

    auto groupLogic() const noexcept -> msg::filter::FilterLogicGadget::FilterLogic {
        return m_group_logic;
    }
    void          setGroupLogic(msg::filter::FilterLogicGadget::FilterLogic);
    Q_SIGNAL void groupLogicChanged();

private:
    virtual void fromVariantlist(const QVariantList& v) = 0;

    Q_SLOT void markDirty();
    bool        m_dirty;
    msg::filter::FilterLogicGadget::FilterLogic m_group_logic {
        msg::filter::FilterLogicGadget::FilterLogic::FILTER_LOGIC_UNSPECIFIED
    };
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

