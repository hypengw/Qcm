#pragma once

#include <QtQml/QQmlEngine>
#include "Qcm/backend.hpp"
#include "Qcm/util/async.hpp"
#include "Qcm/query/query.hpp"

namespace qcm
{

class QrAuthUrlQuery : public Query, public QueryExtra<msg::QrAuthUrlRsp, QrAuthUrlQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString typeName READ typeName WRITE setTypeName NOTIFY typeNameChanged)

public:
    QrAuthUrlQuery(QObject* parent = nullptr);
    void reload() override;
    auto typeName() const -> QString;
    void setTypeName(const QString&);

    Q_SIGNAL void typeNameChanged();

private:
    QString m_type_name;
};
} // namespace qcm