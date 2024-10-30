#pragma once

// workaround for QTBUG-83160
#if defined(Q_MOC_RUN)
#    define __cplusplus 202002
#endif

#include <ranges>
#include <vector>
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

    Q_INVOKABLE virtual QVariant item(int index) const = 0;
};

template<typename TItem, typename IMPL>
class QMetaListModelPre : public QMetaListModelBase {
public:
    using value_type = TItem;
    using param_type = std::conditional_t<std::is_pointer_v<TItem>, TItem, const TItem&>;

    QMetaListModelPre(QObject* parent = nullptr): QMetaListModelBase(parent) {};
    virtual ~QMetaListModelPre() {};

    void insert(int index, param_type item) { insert(index, std::array { item }); }
    template<typename T>
        requires std::ranges::sized_range<T>
    // std::same_as<std::decay_t<typename T::value_type>, TItem>
    void insert(int index, const T& range) {
        auto size = range.size();
        if (size < 1) return;
        beginInsertRows({}, index, index + size - 1);
        crtp_impl().insert_impl(index, std::begin(range), std::end(range));
        endInsertRows();
    }

    void remove(int index, int size = 1) {
        if (size < 1) return;
        auto last = index + size;
        beginRemoveRows({}, index, last - 1);
        crtp_impl().erase_impl(index, last);
        endRemoveRows();
    }
    void replace(int row, param_type item) {
        crtp_impl().assign(row, item);
        auto idx = index(row);
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
    virtual int rowCount(const QModelIndex& = QModelIndex()) const override {
        return crtp_impl().size();
    }

    template<typename T, typename IdFn>
        requires std::ranges::sized_range<T> &&
                 std::same_as<std::decay_t<typename T::value_type>, TItem> &&
                 std::convertible_to<std::invoke_result_t<IdFn, TItem>, std::string>
    void convertModel(const T& items, IdFn&& id_fn) {
        std::set<std::string> id_set;
        for (auto& el : items) {
            id_set.insert(id_fn(el));
        }

        for (std::size_t i = 0; i < crtp_impl().size();) {
            auto id = id_fn(crtp_impl().at(i));
            if (id_set.contains(id))
                i++;
            else
                remove(i);
        }

        for (std::size_t i = 0; i < items.size(); i++) {
            auto in = items[i];
            if ((std::size_t)rowCount() == i) {
                insert(i, in);
            } else {
                auto& out = crtp_impl().at(i);
                if (id_fn(in) != id_fn(out))
                    insert(i, in);
                else if (in != out)
                    replace(i, in);
            }
        }

        for (std::size_t i = items.size(); i < crtp_impl().size(); i++) {
            remove(i);
        }
    }

private:
    auto&       crtp_impl() { return *static_cast<IMPL*>(this); }
    const auto& crtp_impl() const { return *static_cast<const IMPL*>(this); }
};

template<typename TItem, typename CRTP, typename Allocator = std::allocator<TItem>>
class QMetaListModel : public QMetaListModelPre<TItem, CRTP> {
    friend class QMetaListModelPre<TItem, CRTP>;

public:
    using base_type      = QMetaListModelPre<TItem, CRTP>;
    using allocator_type = Allocator;
    using container_type = std::vector<TItem>;
    using iterator       = container_type::iterator;
    using param_type     = base_type::param_type;

    QMetaListModel(QObject* parent = nullptr, Allocator allc = Allocator())
        : base_type(parent), m_items(allc) {}
    virtual ~QMetaListModel() {}

    auto        begin() const { return std::begin(m_items); }
    auto        end() const { return std::end(m_items); }
    auto        begin() { return std::begin(m_items); }
    auto        end() { return std::end(m_items); }
    auto        size() const { return std::size(m_items); }
    const auto& at(std::size_t idx) const { return m_items.at(idx); }
    auto&       at(std::size_t idx) { return m_items.at(idx); }
    void        assign(std::size_t idx, const TItem& t) { m_items.at(idx) = t; }
    auto        find(param_type t) const { return std::find(begin(), end(), t); }
    auto        find(param_type t) { return std::find(begin(), end(), t); }

protected:
    template<typename Tin>
    void insert_impl(std::size_t it, Tin beg, Tin end) {
        m_items.insert(begin() + it, beg, end);
    }

    void erase_impl(std::size_t index, std::size_t last) {
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
    std::vector<TItem, Allocator> m_items;
};

} // namespace meta_model