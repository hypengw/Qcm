#pragma once

#include "Qcm/model/share_store.hpp"

#include "Qcm/backend_msg.hpp"

namespace qcm
{

class AppStore : public QObject {
    Q_OBJECT
public:
    AppStore(QObject* parent = nullptr);
    meta_model::ItemTrait<qcm::msg::model::Album>::store_type albums;
    meta_model::ItemTrait<qcm::msg::model::Song>::store_type  songs;
};

} // namespace qcm
