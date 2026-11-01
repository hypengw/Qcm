module;
#include "QExtra/macro_qt.hpp"
#ifdef Q_MOC_RUN
#    include "Qcm/status/provider.moc"
#endif

export module qcm:status.provider;
export import :msg;
export import :model.item_id_list;

namespace qcm
{
export class ProviderMetaStatusModel
    : public kstore::QGadgetListModel,
      public kstore::QMetaListModelCRTP<msg::model::ProviderMeta, ProviderMetaStatusModel,
                                        kstore::ListStoreType::Map> {
    Q_OBJECT
    QML_ELEMENT

    using Model = msg::model::ProviderMeta;

public:
    ProviderMetaStatusModel(QObject* parent = nullptr);
    ~ProviderMetaStatusModel();
};

export class LibraryStatus : public QObject {
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(qcm::msg::id::LibraryIds activedIds READ activedIds NOTIFY activedIdsChanged)
public:
    LibraryStatus(QObject* parent = nullptr);
    ~LibraryStatus();
    auto activedIds() const -> msg::id::LibraryIds;

    Q_INVOKABLE bool actived(const qcm::msg::id::LibraryId&) const;
    Q_INVOKABLE void setActived(const qcm::msg::id::LibraryId&, bool);

    Q_SIGNAL void activedChanged(const qcm::msg::id::LibraryId&, bool);
    Q_SIGNAL void activedIdsChanged();

private:
    std::set<i64>         m_inactived;
};

export class ProviderStatusModel
    : public model::ItemIdListModel,
      public kstore::QMetaListModelCRTP<model::ProviderStatus, ProviderStatusModel,
                                        kstore::ListStoreType::Map> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool syncing READ syncing NOTIFY syncingChanged FINAL)
    Q_PROPERTY(
        qcm::LibraryStatus* libraryStatus READ libraryStatus NOTIFY libraryStatusChanged FINAL)
    Q_PROPERTY(qcm::msg::id::ProviderIds activedIds READ activedIds NOTIFY activedIdsChanged FINAL)

public:
    ProviderStatusModel(QObject* parent = nullptr);
    ~ProviderStatusModel();

    void updateSyncStatus(const msg::model::ProviderSyncStatus&);
    auto syncing() const -> bool;
    auto libraryStatus() const -> LibraryStatus*;
    auto activedIds() const -> msg::id::ProviderIds;

    Q_INVOKABLE bool     actived(const qcm::msg::id::ProviderId&) const;
    Q_INVOKABLE void     setActived(const qcm::msg::id::ProviderId&, bool);
    Q_INVOKABLE QVariant itemById(const qcm::msg::id::ProviderId&) const;
    Q_INVOKABLE QVariant metaById(const qcm::msg::id::ProviderId&) const;
    Q_INVOKABLE QString  svgAt(qint32) const;
    Q_INVOKABLE QString  svg(const qcm::msg::id::ProviderId&) const;

    Q_SIGNAL void syncingChanged(bool);
    Q_SIGNAL void providerSyncFinished(const qcm::msg::id::ProviderId&);
    Q_SIGNAL void libraryStatusChanged();
    Q_SIGNAL void activedChanged(const qcm::msg::id::ProviderId&, bool);
    Q_SIGNAL void activedIdsChanged();

private:
    void setSyncing(bool);
    void checkSyncing();

    bool                  m_syncing;
    LibraryStatus*        m_lib_status;
    std::set<i64>      m_inactived;
};

} // namespace qcm
