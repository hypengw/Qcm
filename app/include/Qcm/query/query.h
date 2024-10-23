#pragma once
#include "asio_helper/task.h"
#include "qcm_interface/async.h"
#include "qcm_interface/item_id.h"

namespace qcm::query
{

class SyncAPi {
public:
    static auto sync_item(model::ItemId itemId) -> task<void>;
    static auto sync_collection(enums::CollectionType type) -> task<void>;
};

template<typename T>
class Query : public QAsyncResult {
public:
    Query(QObject* parent = nullptr): QAsyncResult(parent), m_data(new T(this)) {}
    ~Query() {}

    auto data() const -> QVariant override { return QVariant::fromValue(m_data); }
    auto tdata() const -> T* { return m_data; }

private:
    T* m_data;
};
} // namespace qcm::query