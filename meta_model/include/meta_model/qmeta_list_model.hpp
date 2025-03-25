#pragma once

// workaround for QTBUG-83160
#if defined(Q_MOC_RUN)
#    define __cplusplus 202002
#endif

#include <ranges>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <set>

#include <QtCore/QAbstractItemModel>
#include "meta_model/qmeta_model_base.hpp"
#include "meta_model/item_trait.hpp"

namespace meta_model
{

enum class QMetaListStore
{
    Vector = 0,
    VectorWithMap,
    Map
};

namespace detail
{

template<typename T, QMetaListStore>
struct allocator_helper {
    using value_type = T;
};
template<typename T>
struct allocator_helper<T, QMetaListStore::Map> {
    using value_type = std::pair<const usize, T>;
};

template<typename T, QMetaListStore S>
using allocator_value_type = allocator_helper<T, S>::value_type;

template<typename Allocator, typename T>
using rebind_alloc = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;

template<typename T, typename Allocator, QMetaListStore Store>
class ListImpl;

class QMetaListModelBase : public QMetaModelBase<QAbstractListModel> {
    Q_OBJECT
public:
    QMetaListModelBase(QObject* parent = nullptr);
    virtual ~QMetaListModelBase();

    Q_INVOKABLE virtual QVariant     item(qint32 index) const                      = 0;
    Q_INVOKABLE virtual QVariantList items(qint32 offset = 0, qint32 n = -1) const = 0;
};

template<typename TItem, QMetaListStore Store, typename Allocator, typename IMPL>
class QMetaListModelPre : public QMetaListModelBase {
public:
    using value_type = TItem;
    template<typename T>
    using rebind_alloc = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;

    QMetaListModelPre(QObject* parent = nullptr): QMetaListModelBase(parent) {};
    virtual ~QMetaListModelPre() {};

    template<typename T>
        requires std::same_as<std::remove_cvref_t<T>, TItem>
    void insert(int index, T&& item) {
        insert(index, std::array { std::forward<T>(item) });
    }
    template<typename T>
        requires std::ranges::sized_range<T>
    // std::same_as<std::decay_t<typename T::value_type>, TItem>
    void insert(int index, T&& range) {
        auto size = range.size();
        if (size < 1) return;
        beginInsertRows({}, index, index + size - 1);
        auto begin = std::begin(range);
        auto end   = std::end(range);
        if constexpr (std::same_as<decltype(begin), decltype(end)>) {
            crtp_impl()._insert_impl(index, begin, end);
        } else {
            crtp_impl()._insert_impl(index, std::forward<T>(range));
        }
        endInsertRows();
    }
    void remove(int index, int size = 1) {
        if (size < 1) return;
        removeRows(index, size);
    }
    auto removeRows(int row, int count, const QModelIndex& parent = {}) -> bool override {
        if (count < 1) return false;
        beginRemoveRows(parent, row, row + count - 1);
        crtp_impl()._erase_impl(row, row + count);
        endRemoveRows();
        return true;
    }
    template<typename Func>
    void remove_if(Func&& func) {
        std::set<int, std::greater<>> indexes;
        for (int i = 0; i < rowCount(); i++) {
            auto& el = crtp_impl().at(i);
            if (func(el)) {
                indexes.insert(i);
            }
        }
        for (auto& i : indexes) {
            removeRow(i);
        }
    }
    void replace(int row, param_type<TItem> val) {
        auto& item = crtp_impl().at(row);
        item       = val;
        auto idx   = index(row);
        dataChanged(idx, idx);
    }

    void resetModel() {
        beginResetModel();
        crtp_impl()._reset_impl();
        endResetModel();
    }

    template<typename T>
        requires std::ranges::sized_range<T>
    void resetModel(const std::optional<T>& items) {
        beginResetModel();
        if (items) {
            crtp_impl()._reset_impl(items.value());
        } else {
            crtp_impl()._reset_impl();
        }
        endResetModel();
    }

    template<typename T>
        requires std::ranges::sized_range<T>
    // std::same_as<std::decay_t<typename T::value_type>, TItem>
    void resetModel(const T& items) {
        beginResetModel();
        crtp_impl()._reset_impl(items);
        endResetModel();
    }
    template<typename T>
        requires std::ranges::sized_range<T>
    void replaceResetModel(const T& items) {
        auto  size = items.size();
        usize old  = std::max(rowCount(), 0);
        auto  num  = std::min<int>(old, size);
        for (auto i = 0; i < num; i++) {
            crtp_impl().at(i) = items[i];
        }
        if (num > 0) dataChanged(index(0), index(num));
        if (size > old) {
            insert(num, std::ranges::subrange(items.begin() + num, items.end(), size - num));
        } else if (size < old) {
            removeRows(size, old - size);
        }
    }

    QVariant item(int idx) const override {
        if ((usize)std::max(idx, 0) >= crtp_impl().size()) return {};
        if constexpr (is_specialization_of_v<value_type, std::variant>) {
            return std::visit(
                [](const auto& v) -> QVariant {
                    return QVariant::fromValue(v);
                },
                crtp_impl().at(idx));
        } else {
            return QVariant::fromValue(crtp_impl().at(idx));
        }
    }

    auto items(qint32 offset = 0, qint32 n = -1) const -> QVariantList override {
        if (n == -1) n = rowCount();
        auto view = std::views::transform(std::views::iota(offset, n), [this](qint32 idx) {
            return item(idx);
        });
        return QVariantList { view.begin(), view.end() };
    }

    virtual int rowCount(const QModelIndex& = QModelIndex()) const override {
        return crtp_impl().size();
    }

private:
    auto&       crtp_impl() { return *static_cast<IMPL*>(this); }
    const auto& crtp_impl() const { return *static_cast<const IMPL*>(this); }
};
} // namespace detail

namespace detail
{
template<typename T, typename TItem>
concept syncable_list =
    std::ranges::sized_range<T> &&
    std::same_as<std::remove_cvref_t<std::ranges::range_value_t<T>>, TItem> && hashable_item<TItem>;

template<typename T, typename Allocator>
class ListImpl<T, Allocator, QMetaListStore::Vector> {
public:
    using allocator_type = Allocator;
    using container_type = std::vector<T, Allocator>;
    using iterator       = container_type::iterator;

    ListImpl(Allocator allc = Allocator()): m_items(allc) {}
    auto        begin() const { return std::begin(m_items); }
    auto        end() const { return std::end(m_items); }
    auto        begin() { return std::begin(m_items); }
    auto        end() { return std::end(m_items); }
    auto        size() const { return std::size(m_items); }
    const auto& at(usize idx) const { return m_items.at(idx); }
    auto&       at(usize idx) { return m_items.at(idx); }
    auto        find(param_type<T> t) const { return std::find(begin(), end(), t); }
    auto        find(param_type<T> t) { return std::find(begin(), end(), t); }
    auto        get_allocator() const { return m_items.get_allocator(); }

protected:
    template<typename Tin>
    void _insert_impl(usize idx, Tin beg, Tin end) {
        m_items.insert(begin() + idx, beg, end);
    }

    template<std::ranges::range U>
    void _insert_impl(usize idx, U&& range) {
        std::ranges::copy(std::forward<U>(range), std::insert_iterator(m_items, begin() + idx));
    }

    void _erase_impl(usize index, usize last) {
        auto it = m_items.begin();
        m_items.erase(it + index, it + last);
    }

    void _reset_impl() { m_items.clear(); }

    template<std::ranges::range U>
    void _reset_impl(const U& items) {
        m_items.clear();
        _insert_impl(0, std::begin(items), std::end(items));
    }

private:
    container_type m_items;
};

template<typename T, typename Allocator>
class ListImpl<T, Allocator, QMetaListStore::VectorWithMap> {
public:
    using allocator_type = Allocator;
    using container_type = std::vector<T, Allocator>;
    using key_type       = ItemTrait<T>::key_type;
    using iterator       = container_type::iterator;

    ListImpl(Allocator allc = Allocator()): m_map(allc), m_items(allc) {}

    auto        begin() const { return std::begin(m_items); }
    auto        end() const { return std::end(m_items); }
    auto        begin() { return std::begin(m_items); }
    auto        end() { return std::end(m_items); }
    auto        size() const { return std::size(m_items); }
    const auto& at(usize idx) const { return m_items.at(idx); }
    auto&       at(usize idx) { return m_items.at(idx); }
    auto        find(param_type<T> t) const { return std::find(begin(), end(), t); }
    auto        find(param_type<T> t) { return std::find(begin(), end(), t); }
    auto        get_allocator() const { return m_items.get_allocator(); }

    // hash
    auto        contains(param_type<T> t) const { return m_map.contains(hash(t)); }
    const auto& value_at(param_type<key_type> key) const { return m_items.at(m_map.at(key)); };
    auto&       value_at(param_type<key_type> key) { return m_items.at(m_map.at(key)); };
    auto        idx_at(usize hash) const -> usize { return m_map.at(hash); };

protected:
    template<typename Tin>
    void _insert_impl(usize idx, Tin beg, Tin end) {
        auto size = m_items.size();
        m_items.insert(begin() + idx, beg, end);
        for (auto i = idx; i < m_items.size(); i++) {
            m_map.insert_or_assign(ItemTrait<T>::key(m_items.at(i)), i);
        }
    }

    template<std::ranges::range U>
    void _insert_impl(usize idx, U&& range) {
        std::ranges::copy(std::forward<U>(range), std::insert_iterator(m_items, begin() + idx));
        for (auto i = idx; i < m_items.size(); i++) {
            m_map.insert_or_assign(ItemTrait<T>::key(m_items.at(i)), i);
        }
    }

    void _erase_impl(usize idx, usize last) {
        auto it = m_items.begin();
        for (auto i = idx; i < last; i++) {
            m_map.erase(ItemTrait<T>::key(m_items.at(i)));
        }
        m_items.erase(it + idx, it + last);
    }

    void _reset_impl() {
        m_items.clear();
        m_map.clear();
    }

    template<std::ranges::range U>
    void _reset_impl(U&& items) {
        m_items.clear();
        m_map.clear();
        _insert_impl(0, std::forward<U>(items).begin(), std::forward<U>(items).end());
    }

    auto& _maps() { return m_map; }

private:
    using idx_hash_type =
        std::unordered_map<key_type, usize, std::hash<key_type>, std::equal_to<key_type>,
                           detail::rebind_alloc<allocator_type, std::pair<const key_type, usize>>>;
    // indexed cache with hash
    idx_hash_type  m_map;
    container_type m_items;
};
template<typename T, typename Allocator>
class ListImpl<T, Allocator, QMetaListStore::Map> {
public:
    using allocator_type = Allocator;
    using key_type       = ItemTrait<T>::key_type;
    using container_type =
        std::unordered_map<key_type, T, std::hash<key_type>, std::equal_to<key_type>,
                           detail::rebind_alloc<Allocator, std::pair<const key_type, T>>>;
    using iterator = container_type::iterator;

    ListImpl(Allocator allc = Allocator()): m_order(allc), m_items(allc) {}

    auto        begin() const { return std::begin(m_items); }
    auto        end() const { return std::end(m_items); }
    auto        begin() { return std::begin(m_items); }
    auto        end() { return std::end(m_items); }
    auto        size() const { return std::size(m_items); }
    const auto& at(usize idx) const { return value_at(m_order.at(idx)); }
    auto&       at(usize idx) { return value_at(m_order.at(idx)); }
    // auto        find(param_type t) const { return std::find(begin(), end(), t); }
    // auto        find(param_type t) { return std::find(begin(), end(), t); }
    auto get_allocator() const { return m_order.get_allocator(); }

    // hash
    auto        contains(param_type<T> t) const { return m_items.contains(hash(t)); }
    auto        key_at(usize idx) const { return m_order.at(idx); }
    const auto& value_at(param_type<key_type> key) const { return m_items.at(key); };
    auto&       value_at(param_type<key_type> key) { return m_items.at(key); };

protected:
    template<typename Tin>
    void _insert_impl(usize it, Tin beg, Tin end) {
        std::vector<key_type, detail::rebind_alloc<allocator_type, key_type>> order(
            get_allocator());
        for (auto it = beg; it != end; it++) {
            auto k = ItemTrait<T>::key(*it);
            order.emplace_back(k);
            m_items.insert_or_assign(k, *it);
        }
        m_order.insert(m_order.begin() + it, order.begin(), order.end());
    }

    template<std::ranges::range U>
    void _insert_impl(usize it, U&& range) {
        std::vector<key_type, detail::rebind_alloc<allocator_type, key_type>> order(
            get_allocator());
        for (auto&& el : std::forward<U>(range)) {
            auto k = hash(el);
            order.emplace_back(k);
            m_items.insert_or_assign(k, std::forward<decltype(el)>(el));
        }
        m_order.insert(m_order.begin() + it, order.begin(), order.end());
    }

    void _erase_impl(usize index, usize last) {
        auto it    = m_order.begin();
        auto begin = it + index;
        auto end   = it + last;
        for (auto it = begin; it != end; it++) {
            m_items.erase(*it);
        }
        m_order.erase(it + index, it + last);
    }

    void _reset_impl() {
        m_items.clear();
        m_order.clear();
    }

    template<std::ranges::range U>
    void _reset_impl(U&& items) {
        m_order.clear();
        m_items.clear();
        _insert_impl(0, std::forward<U>(items).begin(), std::forward<U>(items).end());
    }

private:
    std::vector<key_type, detail::rebind_alloc<allocator_type, key_type>> m_order;

    container_type m_items;
};

} // namespace detail

template<typename TItem, typename CRTP, QMetaListStore Store,
         typename Allocator = std::allocator<detail::allocator_value_type<TItem, Store>>>
class QMetaListModel : public detail::QMetaListModelPre<TItem, Store, Allocator, CRTP>,
                       public detail::ListImpl<TItem, Allocator, Store> {
    friend class detail::QMetaListModelPre<TItem, Store, Allocator, CRTP>;
    using base_type      = detail::QMetaListModelPre<TItem, Store, Allocator, CRTP>;
    using base_impl_type = detail::ListImpl<TItem, Allocator, Store>;

public:
    using allocator_type = Allocator;
    using container_type = base_impl_type::container_type;
    using iterator       = container_type::iterator;

    template<typename T>
    using rebind_alloc = detail::rebind_alloc<allocator_type, T>;

    QMetaListModel(QObject* parent = nullptr, Allocator allc = Allocator())
        : base_type(parent), base_impl_type(allc) {}
    virtual ~QMetaListModel() {}

    template<detail::syncable_list<TItem> U>
    void sync(U&& items) {
        using key_type     = ItemTrait<TItem>::key_type;
        using idx_set_type = std::set<usize, std::less<>, rebind_alloc<usize>>;
        using idx_map_type = std::unordered_map<key_type,
                                                usize,
                                                std::hash<key_type>,
                                                std::equal_to<key_type>,
                                                rebind_alloc<std::pair<const key_type, usize>>>;

        // get key to idx map
        idx_map_type key_to_idx(this->get_allocator());
        key_to_idx.reserve(items.size());
        for (usize i = 0; i < items.size(); ++i) {
            auto h = ItemTrait<TItem>::key(items[i]);
            key_to_idx.insert({ h, i });
        }

        // update and remove
        if constexpr (Store == QMetaListStore::Vector) {
            for (usize i = 0; i < this->size();) {
                auto key = ItemTrait<TItem>::key(this->at(i));
                if (auto it = key_to_idx.find(key); it != key_to_idx.end()) {
                    this->at(i) = std::forward<U>(items)[it->second];
                    key_to_idx.erase(it);
                    ++i;
                } else {
                    this->remove(i);
                }
            }
        } else if constexpr (Store == QMetaListStore::VectorWithMap) {
            for (auto& el : this->_maps()) {
                if (auto it = key_to_idx.find(el.first); it != key_to_idx.end()) {
                    at(el.second) = std::forward<U>(items)[it->second];
                    key_to_idx.erase(it);
                } else {
                    this->remove(el.second);
                }
            }
        } else if constexpr (Store == QMetaListStore::Map) {
            for (usize i = 0; i < this->size();) {
                auto h = this->key_at(i);
                if (auto it = key_to_idx.find(h); it != key_to_idx.end()) {
                    this->at(i) = std::forward<U>(items)[it->second];
                    key_to_idx.erase(it);
                    ++i;
                } else {
                    this->remove(i);
                }
            }
        }

        // insert new
        idx_set_type ids(this->get_allocator());
        for (auto& el : key_to_idx) {
            ids.insert(el.second);
        }
        for (auto id : ids) {
            this->insert(id, std::forward<U>(items)[id]);
        }
    }
};

} // namespace meta_model