#pragma once
#include <map>
#include <memory>
#include <utility>

namespace qcm::store
{
template<class Key, class Flight>
class LoadPool {
public:
    explicit LoadPool(std::size_t capacity = 128): m_capacity(capacity) {}
    std::shared_ptr<Flight> find(const Key& key) const {
        auto it = m_active.find(key);
        return it == m_active.end() ? nullptr : it->second;
    }
    template<class Factory>
    std::pair<std::shared_ptr<Flight>, bool> acquire(const Key& key, Factory create) {
        if (auto existing = find(key)) return { existing, false };
        if (m_active.size() >= m_capacity) return { nullptr, false };
        auto flight = create();
        m_active.emplace(key, flight);
        return { flight, true };
    }
    std::shared_ptr<Flight> take(const Key& key, const std::shared_ptr<Flight>& expected) {
        auto it = m_active.find(key);
        if (it == m_active.end() || it->second != expected) return {};
        auto value = std::move(it->second);
        m_active.erase(it);
        return value;
    }
    auto clear() { return std::exchange(m_active, {}); }

private:
    std::map<Key, std::shared_ptr<Flight>> m_active;
    std::size_t                            m_capacity;
};
} // namespace qcm::store
