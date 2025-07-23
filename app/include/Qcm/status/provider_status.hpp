#pragma once

#include "kstore/qt/gadget_model.hpp"
#include "Qcm/backend_msg.hpp"

namespace qcm
{
class ProviderMetaStatusModel
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

class LibraryStatus : public QObject {
    Q_OBJECT

    Q_PROPERTY(QtProtobuf::int64List activedIds READ activedIds NOTIFY activedIdsChanged)
public:
    LibraryStatus(QObject* parent = nullptr);
    ~LibraryStatus();
    auto activedIds() -> const QtProtobuf::int64List&;

    Q_INVOKABLE bool actived(i64 id) const;
    Q_INVOKABLE void setActived(i64 id, bool);

    Q_SIGNAL void activedChanged(i64, bool);
    Q_SIGNAL void activedIdsChanged();

private:
    std::set<i64>         m_inactived;
    QtProtobuf::int64List m_ids;
};

class ProviderStatusModel
    : public kstore::QGadgetListModel,
      public kstore::QMetaListModelCRTP<model::ProviderStatus, ProviderStatusModel,
                                    kstore::ListStoreType::Map> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool syncing READ syncing NOTIFY syncingChanged FINAL)
    Q_PROPERTY(
        qcm::LibraryStatus* libraryStatus READ libraryStatus NOTIFY libraryStatusChanged FINAL)

public:
    ProviderStatusModel(QObject* parent = nullptr);
    ~ProviderStatusModel();

    void updateSyncStatus(const msg::model::ProviderSyncStatus&);
    auto syncing() const -> bool;
    auto libraryStatus() const -> LibraryStatus*;

    Q_INVOKABLE QVariant itemById(const model::ItemId&) const;
    Q_INVOKABLE QVariant metaById(const model::ItemId&) const;
    Q_INVOKABLE QString  svg(qint32) const;
    Q_INVOKABLE QString  svg(const model::ItemId&) const;

    Q_SIGNAL void syncingChanged(bool);
    Q_SIGNAL void libraryStatusChanged();

private:
    void setSyncing(bool);
    void checkSyncing();

    bool           m_syncing;
    LibraryStatus* m_lib_status;
};

} // namespace qcm
