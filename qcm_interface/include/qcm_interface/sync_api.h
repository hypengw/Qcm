#pragma once
#include "asio_helper/task.h"
#include "error/error.h"
#include "qcm_interface/item_id.h"
#include "qcm_interface/enum.h"

namespace qcm::query
{

template<typename T>
using Result = nstd::expected<T, error::Error>;

class QCM_INTERFACE_API SyncAPi {
public:
    static auto sync_item(model::ItemId itemId, bool notify = false) -> task<Result<bool>>;
    static auto sync_items(std::span<const model::ItemId> itemId) -> task<Result<bool>>;
    static auto sync_list(enums::SyncListType type, model::ItemId itemId, i32 offset, i32 limit)
        -> task<Result<i32>>;
    static auto sync_collection(enums::CollectionType type) -> task<Result<bool>>;
};
} // namespace qcm::query