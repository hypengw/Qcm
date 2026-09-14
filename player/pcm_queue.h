#pragma once

#include <array>
#include <atomic>
#include <algorithm>
#include <cstring>
#include <memory_resource>
#include <memory>
#include <cstdint>

namespace player
{
// One producer and one consumer; reset only after both have stopped.
template<class T, std::size_t Capacity>
class SpscQueue {
public:
    explicit SpscQueue(std::pmr::memory_resource* memory)
        : m_memory(memory),
          m_data(static_cast<T*>(memory->allocate(sizeof(T) * Capacity, alignof(T)))) {
        std::uninitialized_value_construct_n(m_data, Capacity);
    }
    ~SpscQueue() {
        std::destroy_n(m_data, Capacity);
        m_memory->deallocate(m_data, sizeof(T) * Capacity, alignof(T));
    }
    SpscQueue(const SpscQueue&) = delete;
    bool push(const T& value) {
        auto write = m_write.load(std::memory_order_relaxed);
        if (write - m_read.load(std::memory_order_acquire) == Capacity) return false;
        m_data[write % Capacity] = value;
        m_write.store(write + 1, std::memory_order_release);
        return true;
    }
    const T* front() const {
        auto read = m_read.load(std::memory_order_relaxed);
        if (read == m_write.load(std::memory_order_acquire)) return nullptr;
        return &m_data[read % Capacity];
    }
    void pop() { m_read.fetch_add(1, std::memory_order_release); }
    bool full() const { return m_write.load() - m_read.load() == Capacity; }
    bool empty() const { return m_write.load() == m_read.load(); }
    void reset() {
        m_read  = 0;
        m_write = 0;
    }

private:
    std::pmr::memory_resource* m_memory;
    T*                         m_data;
    alignas(64) std::atomic<std::size_t> m_read {};
    alignas(64) std::atomic<std::size_t> m_write {};
};
struct PcmBlock {
    std::array<float, 1024 * 2> samples;
    unsigned                    frames {};
    double                      seconds {};
};
struct ClockSpan {
    std::uint64_t output_frame {};
    unsigned      frames {};
    double        seconds {};
};
} // namespace player
