#pragma once

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QMetaProperty>

namespace meta_model
{

struct Empty {
    Q_GADGET
};

namespace detail
{

void update_role_names(QHash<int, QByteArray>& role_names, const QMetaObject& meta);
}

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
        detail::update_role_names(m_role_names, m_meta);
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
} // namespace meta_model
