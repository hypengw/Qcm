#pragma once

#include <mutex>
#include <condition_variable>
#include <optional>
#include <utility>
#include <ranges>
#include <list>
#include <deque>
#include <vector>

#include "core/core.h"

namespace qcm
{

template<typename T>
concept queue_cp =
    std::movable<T> && requires(T t, std::list<typename T::value_type> vs, usize num) {
        { t.push(vs) } -> std::convertible_to<usize>;
        { t.pop() };
        { t.pop(num) };
        { t.push_waiter(num) } -> std::convertible_to<bool>;
        { t.pop_waiter(num) } -> std::convertible_to<bool>;
        { t.clear() };
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
    QueueConcurrent(TArg... args): m_data(make_up<Data>()), m_queue(std::forward<TArg>(args)...) {}
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
        lock_type lock { m_data->mutex };
        while (m_queue.push_waiter(v.size())) {
            m_data->not_full.wait(lock);
        }
        auto ret = m_queue.push(std::forward<T>(v));
        notify_add(v.size());
        return ret;
    }

    auto pop() {
        lock_type lock { m_data->mutex };
        while (m_queue.pop_waiter(1)) {
            m_data->not_empty.wait(lock);
        }
        auto ret = m_queue.pop();
        notify_remove(1);
        return ret;
    }

    auto pop(usize num) {
        lock_type lock { m_data->mutex };
        while (m_queue.pop_waiter(num)) {
            m_data->not_empty.wait(lock);
        }
        auto ret = m_queue.pop(num);
        notify_remove(num);
        return ret;
    }

    auto try_push(value_type&& v) {
        lock_type lock { m_data->mutex };
        auto      ret = m_queue.push(std::array { std::move(v) });
        notify_add(1);
        return ret;
    }
    auto try_pop() {
        lock_type lock { m_data->mutex };
        auto      ret = m_queue.pop();
        notify_remove(1);
        return ret;
    }

    void clear() {
        lock_type lock { m_data->mutex };
        queue().clear();
        clear_wait();
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
    Queue    m_queue;
};

template<typename V>
struct QueueWithSize {
public:
    using value_type = V;

    QueueWithSize(usize max_size): max_size(max_size), aborted(false) {}

    template<typename T>
        requires std::ranges::forward_range<T> && std::ranges::sized_range<T>
    usize push(T&& vs) {
        if (max_size > queue.size()) {
            usize num = std::min(max_size - queue.size(), vs.size());
            auto  it  = std::make_move_iterator(std::begin(vs));
            queue.insert(queue.end(), it, it + num);
            return num;
        }
        return 0;
    }
    std::optional<value_type> pop() {
        if (! queue.empty()) {
            value_type back = std::move(queue.front());
            queue.pop_front();
            return back;
        }
        return std::nullopt;
    };
    std::vector<value_type> pop(usize num) {
        std::vector<value_type> out;
        num = std::min(num, queue.size());
        for (usize i = 0; i < num; i++) out.emplace_back(pop().value());
        return out;
    };

    bool push_waiter(usize num) { return num + queue.size() > max_size && ! aborted; }
    bool pop_waiter(usize num) { return num > queue.size() && ! aborted; }

    void clear() { queue.clear(); }

    std::deque<value_type> queue;
    usize                  max_size;
    bool                   aborted;
};

} // namespace qcm
