module;
#include "kstore/subscription.hpp"
#include <functional>
#include <optional>
#include <utility>

export module qcm:model.share_store;
import qcm.core;
import qextra;

export namespace qcm
{
using StoreSubscription = kstore::Subscription;

template<class T>
class ReadStore {
public:
    using item_type = T;
    using Callback  = std::function<void(i64, quint64)>;
    class Handle {
    public:
        explicit Handle(ReadStore store): m_store(std::move(store)) {}
        Handle(ReadStore store, i64 key): m_store(std::move(store)), m_key(key) {
            m_store.pin(key);
        }
        Handle(const Handle& other): m_store(other.m_store), m_key(other.m_key) {
            if (m_key) m_store.pin(*m_key);
        }
        Handle(Handle&& other) noexcept
            : m_store(other.m_store), m_key(std::exchange(other.m_key, {})) {}
        Handle& operator=(const Handle& other) {
            if (this != &other) {
                release();
                m_store = other.m_store;
                m_key   = other.m_key;
                if (m_key) m_store.pin(*m_key);
            }
            return *this;
        }
        Handle& operator=(Handle&& other) noexcept {
            if (this != &other) {
                release();
                m_store = other.m_store;
                m_key   = std::exchange(other.m_key, {});
            }
            return *this;
        }
        ~Handle() { release(); }
        auto     key() const { return m_key; }
        auto     item() const -> const T* { return m_key ? m_store.store_query(*m_key) : nullptr; }
        auto     operator->() const -> const T* { return item(); }
        auto     operator*() const -> const T& { return *item(); }
        explicit operator bool() const { return item() != nullptr; }
        bool     operator==(const Handle& other) const {
            return m_key == other.m_key && m_store == other.m_store;
        }

    private:
        void release() {
            if (m_key) m_store.unpin(*m_key);
            m_key.reset();
        }
        ReadStore          m_store;
        std::optional<i64> m_key;
    };
    using store_item_type = Handle;
    ReadStore()           = default;
    ReadStore(const void* owner, std::function<const T*(i64)> query,
              std::function<StoreSubscription(Callback)> subscribe, std::function<void(i64)> pin,
              std::function<void(i64)> unpin)
        : m_owner(owner),
          m_query(std::move(query)),
          m_subscribe(std::move(subscribe)),
          m_pin(std::move(pin)),
          m_unpin(std::move(unpin)) {}
    const T* store_query(i64 key) const { return m_query ? m_query(key) : nullptr; }
    auto     store_item(i64 key) const -> std::optional<Handle> {
        return store_query(key) ? std::optional<Handle>(Handle(*this, key)) : std::nullopt;
    }
    Handle            hold(i64 key) const { return Handle(*this, key); }
    StoreSubscription subscribe(Callback callback) const {
        return m_subscribe ? m_subscribe(std::move(callback)) : StoreSubscription();
    }
    void pin(i64 key) const {
        if (m_pin) m_pin(key);
    }
    void unpin(i64 key) const {
        if (m_unpin) m_unpin(key);
    }
    bool operator==(const ReadStore& other) const { return m_owner == other.m_owner; }

private:
    const void*                                m_owner {};
    std::function<const T*(i64)>               m_query;
    std::function<StoreSubscription(Callback)> m_subscribe;
    std::function<void(i64)>                   m_pin, m_unpin;
};
} // namespace qcm
