#pragma once

#include <QtQml/QQmlEngine>
#include "Qcm/enum.h"

namespace qcm
{

class ItemId2 {
    Q_GADGET

    Q_PROPERTY(enums::IdType type READ type)
    Q_PROPERTY(QString sid READ id)
    Q_PROPERTY(QString provider READ provider)

public:
    auto type() const -> enums::IdType;
    auto id() const -> enums::IdType;
    auto provider() const -> enums::IdType;

private:
    enums::IdType m_type;
    QString       m_provider;
    QString       m_id;
};

} // namespace qcm