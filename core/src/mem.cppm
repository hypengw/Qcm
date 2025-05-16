module;
#include <memory_resource>
#include <atomic>
export module qcm.core:mem;
import :basic;

namespace qcm
{
export class MemoryStatResource : public std::pmr::memory_resource {
public:
    MemoryStatResource(std::pmr::memory_resource* source = nullptr);

    usize current_bytes() const;
    usize peak_bytes() const;
    usize current_block_count() const;
    usize current_largest_block() const;

protected:
    void* do_allocate(usize bytes, usize alignment) override;
    void  do_deallocate(void* ptr, usize bytes, usize alignment) override;
    bool  do_is_equal(const std::pmr::memory_resource& other) const noexcept override;

private:
    std::pmr::memory_resource* m_source;
    std::atomic<usize>         m_current_bytes;
    std::atomic<usize>         m_peak_bytes;
    std::atomic<usize>         m_current_blocks;
    std::atomic<usize>         m_current_largest_block;
};
} // namespace qcm