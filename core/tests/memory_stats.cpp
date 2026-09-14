import qcm.core;
import rstd;

struct RejectAllocator {};
namespace rstd
{
template<>
struct Impl<alloc::Allocator, RejectAllocator> : DefaultInImpl<alloc::Allocator, RejectAllocator> {
    auto allocate(alloc::Layout) const -> Result<alloc::Allocation, alloc::AllocError> {
        return Err(alloc::AllocError {});
    }
    void deallocate(void*, alloc::Layout) const noexcept {}
};
} // namespace rstd

int main() {
    qcm::MemoryStatAllocator memory;
    auto                     allocator = ::alloc::allocator_ref(memory);
    const auto               small =
        rstd::alloc::Layout::from_size_align(rstd::usize(3), rstd::usize(64)).unwrap();
    const auto large  = rstd::alloc::Layout::make<rstd::uint64_t>();
    auto       first  = allocator->allocate(small).unwrap();
    auto       second = allocator->allocate_zeroed(large).unwrap();
    if (memory.current_bytes() != 11 || memory.peak_bytes() != 11 ||
        memory.current_block_count() != 2 || memory.current_largest_block() != 8)
        return 1;
    if (*static_cast<rstd::uint64_t*>(second.pointer) != 0) return 2;
    allocator->deallocate(first.pointer, small);
    allocator->deallocate(second.pointer, large);
    if (memory.current_bytes() || memory.current_block_count() || memory.peak_bytes() != 11)
        return 3;

    RejectAllocator          rejecting;
    qcm::MemoryStatAllocator failed(::alloc::allocator_ref(rejecting));
    if (failed.allocate(small).is_ok() || failed.current_bytes() || failed.peak_bytes() ||
        failed.current_block_count())
        return 4;

    qcm::MemoryStatResource legacy;
    auto*                   block = legacy.allocate(3, 64);
    if (legacy.current_bytes() != 3 || legacy.current_block_count() != 1) return 5;
    legacy.deallocate(block, 3, 64);
    if (legacy.current_bytes() || legacy.current_block_count() || legacy.peak_bytes() != 3)
        return 6;
    return 0;
}
