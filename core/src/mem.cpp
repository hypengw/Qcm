module;
#include <new>
#include <memory_resource>
#include <atomic>
module qcm.core;
import :mem;

namespace qcm
{

MemoryStatResource::MemoryStatResource(std::pmr::memory_resource* source)
    : m_source(source),
      m_current_bytes(0),
      m_peak_bytes(0),
      m_current_blocks(0),
      m_current_largest_block(0) {}

usize MemoryStatResource::current_bytes() const { return m_current_bytes.load(); }
usize MemoryStatResource::peak_bytes() const { return m_peak_bytes.load(); }
usize MemoryStatResource::current_block_count() const { return m_current_blocks.load(); }
usize MemoryStatResource::current_largest_block() const { return m_current_largest_block.load(); }

void* MemoryStatResource::do_allocate(usize bytes, usize alignment) {
    void* ptr = nullptr;
    if (m_source) {
        ptr = m_source->allocate(bytes, alignment);
    } else {
        ptr = ::operator new(bytes, std::align_val_t(alignment));
    }

    m_current_bytes.fetch_add(bytes, std::memory_order_relaxed);
    m_current_blocks.fetch_add(1, std::memory_order_relaxed);

    usize prev_peak = m_peak_bytes.load(std::memory_order_relaxed);
    usize new_total = m_current_bytes.load(std::memory_order_relaxed);
    while (new_total > prev_peak &&
           ! m_peak_bytes.compare_exchange_weak(prev_peak, new_total, std::memory_order_relaxed));

    usize prev_largest = m_current_largest_block.load(std::memory_order_relaxed);
    while (bytes > prev_largest && ! m_current_largest_block.compare_exchange_weak(
                                       prev_largest, bytes, std::memory_order_relaxed));

    return ptr;
}

void MemoryStatResource::do_deallocate(void* ptr, usize bytes, usize alignment) {
    m_current_bytes.fetch_sub(bytes, std::memory_order_relaxed);
    m_current_blocks.fetch_sub(1, std::memory_order_relaxed);

    if (m_source) {
        m_source->deallocate(ptr, bytes, alignment);
    } else {
        ::operator delete(ptr, bytes, std::align_val_t(alignment));
    }
}

bool MemoryStatResource::do_is_equal(const std::pmr::memory_resource& other) const noexcept {
    return this == &other;
}

} // namespace qcm
