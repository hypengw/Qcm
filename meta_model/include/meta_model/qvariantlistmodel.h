#pragma once

#include <variant>
#include "meta_model/qmetaobjectmodel.h"
#include "core/variant_helper.h"

namespace meta_model
{

template<typename... Ts>
class VariantListModel : public QMetaListModelPre<std::variant<Ts...>, VariantListModel<Ts...>> {
    friend class QMetaListModelPre<std::variant<Ts...>, VariantListModel<Ts...>>;

public:
    VariantListModel(QObject* parent = nullptr)
        : QMetaListModelPre<std::variant<Ts...>, VariantListModel<Ts...>>(parent) {}
    virtual ~VariantListModel() {}

    using value_type     = std::variant<Ts...>;
    using container_type = std::variant<std::vector<Ts>...>;
    using iterator       = std::variant<typename std::vector<Ts>::iterator...>;

    template<typename T>
    bool holds_alternative() const {
        return std::holds_alternative<std::vector<T>>(m_items);
    }

    // override
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (auto prop = this->propertyOfRole(role); prop) {
            return std::visit(
                [&index, &prop](auto& items) -> QVariant {
                    return prop.value().readOnGadget(&items.at(index.row()));
                },
                m_items);
        }
        return {};
    };

    auto begin() const {
        return std::visit(
            [](auto& var) -> iterator {
                return std::begin(var);
            },
            m_items);
    }
    auto end() const {
        return std::visit(
            [](auto& var) -> iterator {
                return std::end(var);
            },
            m_items);
    }
    auto begin() {
        return std::visit(
            [](auto& var) -> iterator {
                return std::begin(var);
            },
            m_items);
    }
    auto end() {
        return std::visit(
            [](auto& var) -> iterator {
                return std::end(var);
            },
            m_items);
    }
    auto size() const {
        return std::visit(
            [](auto& var) -> std::size_t {
                return std::size(var);
            },
            m_items);
    }
    auto at(std::size_t idx) const {
        return std::visit(
            [idx](auto& items) -> value_type {
                return items.at(idx);
            },
            m_items);
    }
    void assign(std::size_t idx, value_type v) {
        std::visit(
            [this, idx](auto& var) {
                std::get<std::vector<std::decay_t<decltype(var)>>>(m_items).at(idx) = var;
            },
            v);
    }

private:
    template<typename Tin>
    void insert_impl(std::size_t idx, Tin beg, Tin end) {
        using value_type = Tin::value_type;
        auto& items      = std::get<std::vector<value_type>>(m_items);
        items.insert(std::begin(items) + idx, beg, end);
    }

    void erase_impl(std::size_t begin, std::size_t last) {
        std::visit(
            [this, begin, last](auto& v) {
                auto it = std::begin(v);
                v.erase(it + begin, it + last);
            },
            m_items);
    }

    void reset_impl() {
        std::visit(
            [](auto& v) {
                v.clear();
            },
            m_items);
    }

    template<typename T>
    void reset_impl(const T& items) {
        auto size = items.size();
        std::visit(
            [this, size](auto& v) {
                v.clear();
                v.resize(size);
            },
            m_items);
        insert_impl(0, std::begin(items), std::end(items));
    }

protected:
    void updateRoleNames(const QMetaObject& meta, const std::size_t i) {
        Q_EMIT this->layoutAboutToBeChanged();
        m_items = helper::make_variant<std::vector<Ts>...>(i);
        this->QMetaListModelBase::updateRoleNames(meta);
    }

private:
    container_type m_items;
};

} // namespace meta_model