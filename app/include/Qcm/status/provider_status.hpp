#pragma once

#include "meta_model/qgadget_list_model.hpp"
#include "Qcm/backend_msg.hpp"

namespace qcm
{
class ProviderMetaStatusModel
    : public meta_model::QGadgetListModel<msg::model::ProviderMeta,
                                          meta_model::QMetaListStore::Map> {
    Q_OBJECT
    QML_ELEMENT

    using Model = msg::model::ProviderMeta;
    using Base  = meta_model::QGadgetListModel<Model, meta_model::QMetaListStore::Map>;

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
    : public meta_model::QGadgetListModel<model::ProviderStatus, meta_model::QMetaListStore::Map> {
    Q_OBJECT
    QML_ELEMENT
    using Base =
        meta_model::QGadgetListModel<model::ProviderStatus, meta_model::QMetaListStore::Map>;

    Q_PROPERTY(bool syncing READ syncing NOTIFY syncingChanged)
    Q_PROPERTY(qcm::LibraryStatus* libraryStatus READ libraryStatus NOTIFY libraryStatusChanged)

public:
    ProviderStatusModel(QObject* parent = nullptr);
    ~ProviderStatusModel();

    void updateSyncStatus(const msg::model::ProviderSyncStatus&);
    auto syncing() const -> bool;
    auto libraryStatus() const -> LibraryStatus*;

    Q_INVOKABLE QString svg(qint32) const;
    Q_SIGNAL void       syncingChanged(bool);
    Q_SIGNAL void       libraryStatusChanged();

private:
    void setSyncing(bool);
    void checkSyncing();

    bool           m_syncing;
    LibraryStatus* m_lib_status;
};

} // namespace qcm
