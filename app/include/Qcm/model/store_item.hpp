#pragma once

#include "Qcm/model/share_store.hpp"
#include "Qcm/backend_msg.hpp"

namespace qcm::model
{

template<typename Store, typename CRTP>
class StoreItem : public QObject {
public:
    using store_type      = Store;
    using item_type       = typename store_type::item_type;
    using store_item_type = typename store_type::store_item_type;

    StoreItem(Store store, QObject* parent): QObject(parent), m_item(store) {}
    ~StoreItem() { unreg(); }

    auto item() const -> item_type {
        if (auto it = m_item.item()) {
            return *it;
        }
        return {};
    }
    void setItem(const item_type& v) {
        auto key = kstore::ItemTrait<item_type>::key(v);
        if (key != m_item.key()) {
            m_item = m_item.store().store_insert(v).first;
            m_item.store().store_changed_callback(std::span { &key, 1 }, m_handle ? *m_handle : 0);
            static_cast<CRTP*>(this)->itemChanged();

            unreg();
            m_handle = Some(m_item.store().store_reg_notify([this](auto) {
                static_cast<CRTP*>(this)->itemChanged();
            }));
        }
    }
    auto extra() const -> QQmlPropertyMap* {
        if (auto key = m_item.key()) {
            if (auto extend = m_item.store().query_extend(*key); extend) {
                return extend->extra.get();
            }
        }
        return nullptr;
    }

private:
    auto unreg() {
        if (m_handle) {
            m_item.store().store_unreg_notify(*m_handle);
        }
    }

    rstd::Option<i64> m_handle;
    store_item_type   m_item;
};

class SongStoreItem
    : public StoreItem<kstore::ItemTrait<qcm::model::Song>::store_type, SongStoreItem> {
    Q_OBJECT
    Q_PROPERTY(qcm::model::Song item READ item NOTIFY itemChanged)
public:
    using base_type = StoreItem<kstore::ItemTrait<qcm::model::Song>::store_type, SongStoreItem>;
    SongStoreItem(QObject* parent = nullptr);
    Q_SIGNAL void itemChanged();
};

class AlbumStoreItem
    : public StoreItem<kstore::ItemTrait<qcm::model::Album>::store_type, AlbumStoreItem> {
    Q_OBJECT
    Q_PROPERTY(qcm::model::Album item READ item NOTIFY itemChanged)
public:
    using base_type = StoreItem<kstore::ItemTrait<qcm::model::Album>::store_type, AlbumStoreItem>;
    AlbumStoreItem(QObject* parent = nullptr);
    Q_SIGNAL void itemChanged();
};

class ArtistStoreItem
    : public StoreItem<kstore::ItemTrait<qcm::model::Artist>::store_type, ArtistStoreItem> {
    Q_OBJECT
    Q_PROPERTY(qcm::model::Artist item READ item NOTIFY itemChanged)
public:
    using base_type = StoreItem<kstore::ItemTrait<qcm::model::Artist>::store_type, ArtistStoreItem>;
    ArtistStoreItem(QObject* parent = nullptr);
    Q_SIGNAL void itemChanged();
};

class MixStoreItem
    : public StoreItem<kstore::ItemTrait<qcm::model::Mix>::store_type, MixStoreItem> {
    Q_OBJECT
    Q_PROPERTY(qcm::model::Mix item READ item NOTIFY itemChanged)
public:
    using base_type = StoreItem<kstore::ItemTrait<qcm::model::Mix>::store_type, MixStoreItem>;
    MixStoreItem(QObject* parent = nullptr);
    Q_SIGNAL void itemChanged();
};

} // namespace qcm::model