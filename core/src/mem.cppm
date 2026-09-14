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
    usize current_largest_block() const;

protected:
    void record_allocation(usize bytes) const;
    void record_deallocation(usize bytes) const;

private:
    mutable std::atomic<usize> m_current_bytes {};
    mutable std::atomic<usize> m_peak_bytes {};
    mutable std::atomic<usize> m_current_blocks {};
    mutable std::atomic<usize> m_current_largest_block {};
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

export class MemoryStatAllocator : public MemoryStats {
public:
    explicit MemoryStatAllocator(rstd::ref<rstd::dyn<rstd::alloc::Allocator>> source =
                                     ::alloc::allocator_ref(::alloc::GLOBAL));

    auto allocate(rstd::alloc::Layout layout) const
        -> rstd::Result<rstd::alloc::Allocation, rstd::alloc::AllocError>;
    void deallocate(void* ptr, rstd::alloc::Layout layout) const noexcept;

private:
    rstd::ref<rstd::dyn<rstd::alloc::Allocator>> m_source;
};
} // namespace qcm

namespace rstd
{
template<>
struct Impl<alloc::Allocator, qcm::MemoryStatAllocator>
    : DefaultInImpl<alloc::Allocator, qcm::MemoryStatAllocator> {
    auto allocate(alloc::Layout layout) const -> Result<alloc::Allocation, alloc::AllocError> {
        return this->self().allocate(layout);
    }
    void deallocate(void* ptr, alloc::Layout layout) const noexcept {
        this->self().deallocate(ptr, layout);
    }
};
} // namespace rstd
