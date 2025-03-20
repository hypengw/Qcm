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

#include <QtCore/QAbstractListModel>
#include <QtCore/QMetaProperty>

#include "core/core.h"

namespace meta_model
{

struct Empty {
    Q_GADGET
};

void update_role_names(QHash<int, QByteArray>& role_names, const QMetaObject& meta);

template<typename TBase>
class QMetaModelBase : public TBase {
public:
    QMetaModelBase(QObject* parent = nullptr): TBase(parent), m_meta(Empty::staticMetaObject) {}
    ~QMetaModelBase() {}
    QHash<int, QByteArray> roleNames() const override { return m_role_names; }
    const QMetaObject&     meta() const { return m_meta; }
    auto                   roleOf(QByteArrayView name) const -> int {
        if (auto it = std::find_if(roleNamesRef().keyValueBegin(),
                                   roleNamesRef().keyValueEnd(),
                                   [name](const auto& el) -> bool {
                                       return el.second == name;
                                   });
            it != roleNamesRef().keyValueEnd()) {
            return it->first;
        }
        return -1;
    }

protected:
    void updateRoleNames(const QMetaObject& meta) {
        this->layoutAboutToBeChanged();
        m_meta = meta;
        meta_model::update_role_names(m_role_names, m_meta);
        this->layoutChanged();
    }
    const QHash<int, QByteArray>& roleNamesRef() const { return m_role_names; }
    auto                          propertyOfRole(int role) const -> std::optional<QMetaProperty> {
        if (auto prop_idx = meta().indexOfProperty(roleNamesRef().value(role).constData());
            prop_idx != -1) {
            return meta().property(prop_idx);
        }
        return std::nullopt;
    }

private:
    QHash<int, QByteArray> m_role_names;
    QMetaObject            m_meta;
};

class QMetaListModelBase : public QMetaModelBase<QAbstractListModel> {
    Q_OBJECT
public:
    QMetaListModelBase(QObject* parent = nullptr);
    virtual ~QMetaListModelBase();

    Q_INVOKABLE virtual QVariant     item(qint32 index) const                      = 0;
    Q_INVOKABLE virtual QVariantList items(qint32 offset = 0, qint32 n = -1) const = 0;
};

enum class QMetaListStore
{
    Vector = 0,
    VectorWithMap,
    Map
};

template<typename TItem, QMetaListStore Store, typename Allocator, typename IMPL>
class QMetaListModelPre : public QMetaListModelBase {
public:
    using value_type = TItem;
    using param_type = std::conditional_t<std::is_pointer_v<TItem>, TItem, const TItem&>;
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
            crtp_impl().insert_impl(index, begin, end);
        } else {
            crtp_impl().insert_impl(index, std::forward<T>(range));
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
        crtp_impl().erase_impl(row, row + count);
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
    void replace(int row, param_type val) {
        auto& item = crtp_impl().at(row);
        item       = val;
        auto idx   = index(row);
        dataChanged(idx, idx);
    }

    void resetModel() {
        beginResetModel();
        crtp_impl().reset_impl();
        endResetModel();
    }

    template<typename T>
        requires std::ranges::sized_range<T>
    void resetModel(const std::optional<T>& items) {
        beginResetModel();
        if (items) {
            crtp_impl().reset_impl(items.value());
        } else {
            crtp_impl().reset_impl();
        }
        endResetModel();
    }

    template<typename T>
        requires std::ranges::sized_range<T>
    // std::same_as<std::decay_t<typename T::value_type>, TItem>
    void resetModel(const T& items) {
        beginResetModel();
        crtp_impl().reset_impl(items);
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
        if constexpr (ycore::is_specialization_of_v<value_type, std::variant>) {
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

} // namespace detail

template<typename TItem, typename CRTP, QMetaListStore Store = QMetaListStore::Vector,
         typename Allocator = std::allocator<detail::allocator_value_type<TItem, Store>>>
class QMetaListModel : public QMetaListModelPre<TItem, Store, Allocator, CRTP> {
    friend class QMetaListModelPre<TItem, Store, Allocator, CRTP>;

public:
    using base_type      = QMetaListModelPre<TItem, Store, Allocator, CRTP>;
    using allocator_type = Allocator;
    using container_type = std::vector<TItem, Allocator>;
    using iterator       = container_type::iterator;
    using param_type     = base_type::param_type;
    template<typename T>
    using rebind_alloc = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;

    QMetaListModel(QObject* parent = nullptr, Allocator allc = Allocator())
        : base_type(parent), m_items(allc) {}
    virtual ~QMetaListModel() {}

    auto        begin() const { return std::begin(m_items); }
    auto        end() const { return std::end(m_items); }
    auto        begin() { return std::begin(m_items); }
    auto        end() { return std::end(m_items); }
    auto        size() const { return std::size(m_items); }
    const auto& at(usize idx) const { return m_items.at(idx); }
    auto&       at(usize idx) { return m_items.at(idx); }
    auto        find(param_type t) const { return std::find(begin(), end(), t); }
    auto        find(param_type t) { return std::find(begin(), end(), t); }
    auto        get_allocator() const { return m_items.get_allocator(); }

    template<typename T, typename IdFn>
        requires std::ranges::sized_range<T> &&
                 std::same_as<std::decay_t<typename T::value_type>, TItem> &&
                 std::convertible_to<std::invoke_result_t<IdFn, TItem>, std::string>
    void sync(const T& items, IdFn&& id_fn) {
        std::set<std::string, std::less<>, rebind_alloc<std::string>> id_set(get_allocator());
        for (auto& el : items) {
            id_set.insert(id_fn(el));
        }

        // remove
        for (usize i = 0; i < size();) {
            auto id = id_fn(at(i));
            if (id_set.contains(id))
                i++;
            else
                this->remove(i);
        }

        // insert and replace
        for (usize i = 0; i < items.size(); i++) {
            auto in = items[i];
            if ((usize)this->rowCount() == i) {
                this->insert(i, in);
            } else {
                auto& out = at(i);
                if (id_fn(in) != id_fn(out))
                    this->insert(i, in);
                else if (in != out)
                    this->replace(i, in);
            }
        }

        // remove last
        for (usize i = items.size(); i < size(); i++) {
            this->remove(i);
        }
    }

protected:
    template<typename Tin>
    void insert_impl(usize idx, Tin beg, Tin end) {
        m_items.insert(begin() + idx, beg, end);
    }

    template<typename TRange>
    void insert_impl(usize idx, TRange&& range) {
        std::ranges::copy(std::forward<TRange>(range),
                          std::insert_iterator(m_items, begin() + idx));
    }

    void erase_impl(usize index, usize last) {
        auto it = m_items.begin();
        m_items.erase(it + index, it + last);
    }

    void reset_impl() { m_items.clear(); }

    template<typename T>
    void reset_impl(const T& items) {
        m_items.clear();
        insert_impl(0, std::begin(items), std::end(items));
    }

private:
    container_type m_items;
};

template<typename TItem, typename Allocator, typename CRTP>
class QMetaListModel<TItem, CRTP, QMetaListStore::VectorWithMap, Allocator>
    : public QMetaListModelPre<TItem, QMetaListStore::VectorWithMap, Allocator, CRTP> {
    friend class QMetaListModelPre<TItem, QMetaListStore::VectorWithMap, Allocator, CRTP>;

public:
    using base_type      = QMetaListModelPre<TItem, QMetaListStore::VectorWithMap, Allocator, CRTP>;
    using allocator_type = Allocator;
    using container_type = std::vector<TItem, Allocator>;
    using iterator       = container_type::iterator;
    using param_type     = base_type::param_type;
    template<typename T>
    using rebind_alloc = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;

    QMetaListModel(QObject* parent = nullptr, Allocator allc = Allocator())
        : base_type(parent), m_map(allc), m_items(allc) {}
    virtual ~QMetaListModel() {}

    auto        begin() const { return std::begin(m_items); }
    auto        end() const { return std::end(m_items); }
    auto        begin() { return std::begin(m_items); }
    auto        end() { return std::end(m_items); }
    auto        size() const { return std::size(m_items); }
    const auto& at(usize idx) const { return m_items.at(idx); }
    auto&       at(usize idx) { return m_items.at(idx); }
    auto        find(param_type t) const { return std::find(begin(), end(), t); }
    auto        find(param_type t) { return std::find(begin(), end(), t); }
    auto        get_allocator() const { return m_items.get_allocator(); }

    // hash
    virtual auto hash(const TItem& t) const noexcept -> usize = 0;
    auto         contains(const TItem& t) const { return m_map.contains(hash(t)); }
    auto         contains_hash(usize hash) const { return m_map.contains(hash); }
    const auto&  value_at(usize hash) const { return m_items.at(m_map.at(hash)); };
    auto&        value_at(usize hash) { return m_items.at(m_map.at(hash)); };
    auto         idx_at(usize hash) const -> usize { return m_map.at(hash); };

    template<typename T>
        requires std::ranges::sized_range<T> &&
                 std::same_as<std::decay_t<typename T::value_type>, TItem>
    void sync(T&& items) {
        idx_hash_type hash_ids(get_allocator());
        hash_ids.reserve(items.size());
        for (usize i = 0; i < items.size(); ++i) {
            auto h = hash(items[i]);
            hash_ids.insert({ h, i });
        }
        {
            auto self_map = m_map;
            for (auto& el : self_map) {
                if (auto it = hash_ids.find(el.first); it != hash_ids.end()) {
                    at(el.second) = std::forward<T>(items)[*it];
                    hash_ids.erase(it);
                } else {
                    this->remove(el.second);
                }
            }
        }
        std::set<usize, std::less<>, rebind_alloc<usize>> ids(get_allocator());
        ids.reserve(hash_ids.size());
        for (auto& el : hash_ids) {
            ids.insert(el.second);
        }
        for (auto id : ids) {
            this->insert(id, std::forward(items)[id]);
        }
    }

protected:
    template<typename Tin>
    void insert_impl(usize idx, Tin beg, Tin end) {
        auto size = m_items.size();
        m_items.insert(begin() + idx, beg, end);
        for (auto i = idx; i < m_items.size(); i++) {
            m_map.insert_or_assign(hash(m_items.at(i)), i);
        }
    }

    template<typename TRange>
    void insert_impl(usize idx, TRange&& range) {
        std::ranges::copy(std::forward<TRange>(range),
                          std::insert_iterator(m_items, begin() + idx));
        for (auto i = idx; i < m_items.size(); i++) {
            m_map.insert_or_assign(hash(m_items.at(i)), i);
        }
    }

    void erase_impl(usize idx, usize last) {
        auto it = m_items.begin();
        for (auto i = idx; i < last; i++) {
            m_map.erase(hash(m_items.at(i)));
        }
        m_items.erase(it + idx, it + last);
    }

    void reset_impl() {
        m_items.clear();
        m_map.clear();
    }

    template<typename T>
    void reset_impl(const T& items) {
        m_items.clear();
        m_map.clear();
        insert_impl(0, std::begin(items), std::end(items));
    }

private:
    using idx_hash_type = std::unordered_map<usize, usize, std::hash<usize>, std::equal_to<usize>,
                                             rebind_alloc<std::pair<const usize, usize>>>;
    // indexed cache with hash
    idx_hash_type  m_map;
    container_type m_items;
};

template<typename TItem, typename Allocator, typename CRTP>
class QMetaListModel<TItem, CRTP, QMetaListStore::Map, Allocator>
    : public QMetaListModelPre<TItem, QMetaListStore::Map, Allocator, CRTP> {
    friend class QMetaListModelPre<TItem, QMetaListStore::Map, Allocator, CRTP>;

public:
    using base_type      = QMetaListModelPre<TItem, QMetaListStore::Map, Allocator, CRTP>;
    using allocator_type = Allocator;
    using container_type =
        std::unordered_map<usize, TItem, std::hash<usize>, std::equal_to<usize>, Allocator>;
    using iterator   = container_type::iterator;
    using param_type = base_type::param_type;
    template<typename T>
    using rebind_alloc = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;

    QMetaListModel(QObject* parent = nullptr, Allocator allc = Allocator())
        : base_type(parent), m_order(allc), m_items(allc) {}
    virtual ~QMetaListModel() {}

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
    virtual auto hash(const TItem& t) const noexcept -> usize = 0;
    auto         contains(const TItem& t) const { return m_items.contains(hash(t)); }
    auto         contains_hash(usize hash) const { return m_items.contains(hash); }
    auto         hash_at(usize idx) const { return m_order.at(idx); }
    const auto&  value_at(usize hash) const { return m_items.at(hash); };
    auto&        value_at(usize hash) { return m_items.at(hash); };

    template<typename T>
        requires std::ranges::sized_range<T> &&
                 std::same_as<std::decay_t<typename T::value_type>, TItem>
    void sync(T&& items) {
        std::
            unordered_map<usize, usize, std::hash<usize>, std::equal_to<usize>, rebind_alloc<usize>>
                hash_ids(get_allocator());
        hash_ids.reserve(items.size());
        for (usize i = 0; i < items.size(); ++i) {
            auto h = hash(items[i]);
            hash_ids.insert({ h, i });
        }
        for (usize i = 0; i < size();) {
            auto h = hash_at(i);
            if (auto it = hash_ids.find(h); it != hash_ids.end()) {
                at(i) = std::forward<T>(items)[*it];
                hash_ids.erase(it);
                ++i;
            } else {
                this->remove(i);
            }
        }
        std::set<usize, std::less<>, rebind_alloc<usize>> ids(get_allocator());
        ids.reserve(hash_ids.size());
        for (auto& el : hash_ids) {
            ids.insert(el.second);
        }
        for (auto id : ids) {
            this->insert(id, std::forward<T>(items)[id]);
        }
    }

protected:
    template<typename Tin>
    void insert_impl(usize it, Tin beg, Tin end) {
        std::vector<usize, rebind_alloc<usize>> order(get_allocator());
        for (auto it = beg; it != end; it++) {
            auto k = hash(*it);
            order.emplace_back(k);
            m_items.insert_or_assign(k, *it);
        }
        m_order.insert(m_order.begin() + it, order.begin(), order.end());
    }

    template<typename TRange>
    void insert_impl(usize it, TRange&& range) {
        std::vector<usize, rebind_alloc<usize>> order(get_allocator());
        for (auto&& el : std::forward<TRange>(range)) {
            auto k = hash(el);
            order.emplace_back(k);
            m_items.insert_or_assign(k, std::forward<decltype(el)>(el));
        }
        m_order.insert(m_order.begin() + it, order.begin(), order.end());
    }

    void erase_impl(usize index, usize last) {
        auto it    = m_order.begin();
        auto begin = it + index;
        auto end   = it + last;
        for (auto it = begin; it != end; it++) {
            m_items.erase(*it);
        }
        m_order.erase(it + index, it + last);
    }

    void reset_impl() {
        m_items.clear();
        m_order.clear();
    }

    template<typename T>
    void reset_impl(const T& items) {
        m_order.clear();
        m_items.clear();
        insert_impl(0, std::begin(items), std::end(items));
    }

private:
    std::vector<usize, rebind_alloc<usize>> m_order;
    container_type                          m_items;
};

} // namespace meta_model