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

class ProviderStatusModel
    : public meta_model::QGadgetListModel<model::ProviderStatus, meta_model::QMetaListStore::Map> {
    Q_OBJECT
    QML_ELEMENT
    using Base =
        meta_model::QGadgetListModel<model::ProviderStatus, meta_model::QMetaListStore::Map>;

    Q_PROPERTY(bool syncing READ syncing NOTIFY syncingChanged)

public:
    ProviderStatusModel(QObject* parent = nullptr);
    ~ProviderStatusModel();

    void updateSyncStatus(const msg::model::ProviderSyncStatus&);
    auto syncing() const -> bool;

    Q_INVOKABLE QString svg(qint32) const;
    Q_SIGNAL void       syncingChanged(bool);

private:
    void setSyncing(bool);
    void checkSyncing();

    bool m_syncing;
};

} // namespace qcm
