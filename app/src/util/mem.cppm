module;
#include <memory_resource>
export module qcm:util.mem;
import qcm.core;

namespace qcm
{
export struct MemResourceMgr {
    using pmr_sync_pool = std::pmr::synchronized_pool_resource;
    MemoryStatResource* pool_stat { new MemoryStatResource {} };
    pmr_sync_pool*      pool { new pmr_sync_pool { pool_stat } };

    MemoryStatResource*  session_mem { new MemoryStatResource { pool } };
    MemoryStatResource*  backend_mem { new MemoryStatResource { pool } };
    MemoryStatAllocator* player_mem { new MemoryStatAllocator {} };
    MemoryStatResource*  store_mem { new MemoryStatResource { pool } };
};
} // namespace qcm
