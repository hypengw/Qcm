#pragma once

#include <mutex>
#include <condition_variable>
#include <optional>
#include <utility>
#include <ranges>
#include <list>
#include <deque>
#include <vector>
#include <cmath>
#include <numeric>
#include <memory_resource>

#include "core/core.h"
#include "core/log.h"

namespace qcm
{

template<typename T>
concept queue_cp =
    std::movable<T> && requires(T t, std::list<typename T::value_type> vs, usize num) {
        { t.push(vs, t.push_info(vs)) } -> std::convertible_to<usize>;
        { t.pop() };
        { t.pop(num) };
        { t.push_info(vs) };
        { t.push_waiter(t.push_info(vs)) } -> std::convertible_to<bool>;
        { t.pop_waiter(num) } -> std::convertible_to<bool>;
        { t.clear() };
        { t.size() } -> std::same_as<usize>;
        { t.is_notify_pop() } -> std::same_as<bool>;
    };

template<typename Queue>
    requires queue_cp<Queue>
class QueueConcurrent : NoCopy {
public:
    using Self           = QueueConcurrent;
    using mutex_type     = std::mutex;
    using lock_type      = std::unique_lock<mutex_type>;
    using condition_type = std::condition_variable;
    using value_type     = Queue::value_type;

    template<typename... TArg>
    QueueConcurrent(TArg... args)
        : m_data(make_up<Data>()), m_temp_push(true), m_queue(std::forward<TArg>(args)...) {}
    ~QueueConcurrent() {}

    QueueConcurrent(Self&& o) { *this = std::move(o); }
    Self& operator=(Self&& o) {
        m_data  = std::exchange(o.m_data, nullptr);
        m_queue = std::move(o.m_queue);
        return *this;
    }

    auto push(value_type&& v) { return push(std::array { std::move(v) }); }

    template<typename T>
        requires std::ranges::forward_range<T> && std::ranges::sized_range<T>
    auto push(T&& v) {
        auto      info = m_queue.push_info(v);
        lock_type lock { m_data->mutex };
        while (m_queue.push_waiter(info) && m_temp_push) {
            m_data->not_full.wait(lock);
        }
        m_temp_push = true;
        auto ret    = m_queue.push(std::forward<T>(v), info);
        notify_add(v.size());
        return ret;
    }

    auto pop() {
        lock_type lock { m_data->mutex };
        while (m_queue.pop_waiter(1)) {
            m_data->not_empty.wait(lock);
        }
        auto ret = m_queue.pop();
        if (queue().is_notify_pop()) notify_remove(1);
        return ret;
    }

    auto pop(usize num) {
        lock_type lock { m_data->mutex };
        while (m_queue.pop_waiter(num)) {
            m_data->not_empty.wait(lock);
        }
        auto ret = m_queue.pop(num);
        if (queue().is_notify_pop()) notify_remove(num);
        return ret;
    }

    auto try_push(value_type&& v) {
        lock_type lock { m_data->mutex };
        auto&&    list = std::array { std::move(v) };
        auto      info = m_queue.push_info(list);
        auto      ret  = m_queue.push(std::move(list), info);
        notify_add(1);
        return ret;
    }
    auto try_pop() {
        lock_type lock { m_data->mutex };
        auto      ret = m_queue.pop();
        if (queue().is_notify_pop()) notify_remove(1);
        return ret;
    }

    void clear() {
        lock_type lock { m_data->mutex };
        queue().clear();
        clear_wait();
    }

    usize size() {
        lock_type lock { m_data->mutex };
        return queue().size();
    }

    void wake_one_pusher() {
        lock_type lock { m_data->mutex };
        m_temp_push = false;
        notify_remove(1);
    }

protected:
    void notify_add(i32 n) {
        for (i32 i = 0; i < n; i++) m_data->not_empty.notify_one();
    }
    void notify_remove(i32 n) {
        for (i32 i = 0; i < n; i++) m_data->not_full.notify_one();
    }

    void clear_wait() {
        m_data->not_empty.notify_all();
        m_data->not_full.notify_all();
    }

    template<typename Oper>
        requires std::invocable<Oper, lock_type&>
    auto with_lock(Oper&& oper) {
        lock_type lock { m_data->mutex };
        return oper(lock);
    }

    Queue& queue() { return m_queue; }

private:
    struct Data {
        mutex_type     mutex;
        condition_type not_empty;
        condition_type not_full;
    };
    up<Data> m_data;
    bool     m_temp_push;
    Queue    m_queue;
};

template<typename V>
struct QueueWithSize {
public:
    using value_type     = V;
    using push_info_type = usize;
    using allocator_type = std::pmr::polymorphic_allocator<V>;

    QueueWithSize(usize max_size, std::pmr::memory_resource* mem = std::pmr::get_default_resource())
        : queue(allocator_type(mem)), data_size(0), max_size(max_size), aborted(false) {}

    template<typename T>
        requires std::ranges::forward_range<T> && std::ranges::sized_range<T>
    usize push(T&& vs, push_info_type info) {
        if (max_size > data_size) {
            usize num = std::min(max_size - queue.size(), vs.size());
            auto  it  = std::make_move_iterator(std::begin(vs));
            queue.insert(queue.end(), it, it + num);
            data_size += info;
            return num;
        }
        return 0;
    }
    std::optional<value_type> pop() {
        if (! queue.empty()) {
            value_type back = std::move(queue.front());
            queue.pop_front();
            data_size -= push_info(back);
            return back;
        }
        return std::nullopt;
    };
    std::vector<value_type> pop(usize num) {
        std::vector<value_type> out;
        num = std::min(num, size());
        for (usize i = 0; i < num; i++) {
            out.emplace_back(pop().value());
            data_size -= push_info(out.back());
        }
        return out;
    };

    push_info_type push_info(const value_type& v) {
        if constexpr (requires(value_type x) { x.size(); }) {
            return v.size();
        } else {
            return 1;
        }
    }

    template<typename T>
        requires(std::ranges::forward_range<T> && std::ranges::sized_range<T> &&
                 std::convertible_to<std::ranges::range_value_t<T>, value_type>)
    push_info_type push_info(const T& list) {
        if constexpr (requires(value_type x) { x.size(); }) {
            return std::accumulate(
                std::begin(list), std::end(list), (usize)0, [](const auto& a, const auto& b) {
                    return a + b.size();
                });
        } else {
            return list.size();
        }
    }

    bool push_waiter(push_info_type size) { return size + data_size > max_size && ! aborted; }
    bool pop_waiter(usize num) { return num > size() && ! aborted; }

    void clear() {
        queue.clear();
        data_size = 0;
    }

    usize size() const { return queue.size(); }

    bool is_notify_pop() const { return data_size < std::max(max_size / 2, (usize)1); }

    std::deque<value_type, allocator_type> queue;

    usize data_size;
    usize max_size;
    bool  aborted;
};

} // namespace qcm
