module;
#include <memory_resource>
#include <atomic>
export module qcm.core:mem;
import :basic;

namespace qcm
{
export class MemoryStats {
public:
    usize current_bytes() const;
    usize peak_bytes() const;
    usize current_block_count() const;

protected:
    void record_allocation(usize bytes) const;
    void record_deallocation(usize bytes) const;

private:
    mutable std::atomic<usize> m_current_bytes {};
    mutable std::atomic<usize> m_peak_bytes {};
    mutable std::atomic<usize> m_current_blocks {};
};

export class MemoryStatResource : public std::pmr::memory_resource, public MemoryStats {
public:
    MemoryStatResource(std::pmr::memory_resource* source = nullptr);

protected:
    void* do_allocate(usize bytes, usize alignment) override;
    void  do_deallocate(void* ptr, usize bytes, usize alignment) override;
    bool  do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

private:
    std::pmr::memory_resource* m_source;
};

} // namespace qcm
