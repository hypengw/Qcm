#pragma once

// workaround for QTBUG-83160
#if defined(Q_MOC_RUN)
#    define __cplusplus 202002
#endif

#include <ranges>
#include <vector>
#include <set>

#include <QtCore/QAbstractListModel>

#include "core/core.h"

namespace meta_model
{
class QMetaListModelBase : public QAbstractListModel {
    Q_OBJECT
public:
    QMetaListModelBase(QObject* parent = nullptr);
    virtual ~QMetaListModelBase();

    virtual QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE virtual QVariant item(int index) const = 0;

protected:
    std::optional<QMetaProperty> propertyOfRole(int role) const;
    void                         updateRoleNames(const QMetaObject&);
    const QMetaObject&           meta() const;

private:
    QHash<int, QByteArray> m_role_names;
    QMetaObject            m_meta;
};

template<typename TItem, typename IMPL>
class QMetaListModelPre : public QMetaListModelBase {
public:
    using value_type = TItem;

    QMetaListModelPre(QObject* parent = nullptr): QMetaListModelBase(parent) {};
    virtual ~QMetaListModelPre() {};

    void insert(int index, const TItem& item) { insert(index, std::array { item }); }
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
    void replace(int row, const TItem& item) {
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
    // std::same_as<std::decay_t<typename T::value_type>, TItem>
    void resetModel(const T& items) {
        beginResetModel();
        crtp_impl().reset_impl(items);
        endResetModel();
    }

    QVariant item(int idx) const override {
        if constexpr (core::is_specialization_of<value_type, std::variant>) {
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

template<typename TItem>
class QMetaListModel : public QMetaListModelPre<TItem, QMetaListModel<TItem>> {
    friend class QMetaListModelPre<TItem, QMetaListModel<TItem>>;

public:
    QMetaListModel(QObject* parent = nullptr)
        : QMetaListModelPre<TItem, QMetaListModel<TItem>>(parent) {}
    virtual ~QMetaListModel() {}

    using container_type = std::vector<TItem>;
    using iterator       = container_type::iterator;

    const auto& begin() const { return std::begin(m_items); }
    const auto& end() const { return std::end(m_items); }
    auto        begin() { return std::begin(m_items); }
    auto        end() { return std::end(m_items); }
    auto        size() const { return std::size(m_items); }
    const auto& at(std::size_t idx) const { return m_items.at(idx); }
    void        assign(std::size_t idx, const TItem& t) { m_items.at(idx) = t; }

private:
    template<typename Tin>
    void insert_impl(std::size_t it, Tin beg, Tin end) {
        m_items.insert(begin() + it, beg, end);
    }

    void erase_impl(std::size_t index, std::size_t last) {
        auto it = m_items.begin();
        m_items.erase(it + index, it + last);
    }

    template<typename T>
    void reset_impl() {
        m_items.clear();
    }

    template<typename T>
    void reset_impl(const T& items) {
        m_items.clear();
        m_items.resize(items.size());
        insert_impl(0, std::begin(items), std::end(items));
    }

private:
    std::vector<TItem> m_items;
};

} // namespace meta_model