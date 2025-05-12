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

    Q_PROPERTY(QString tmpProvider READ tmpProvider WRITE setTmpProvider NOTIFY tmpProviderChanged)

public:
    QrAuthUrlQuery(QObject* parent = nullptr);
    void reload() override;

    auto tmpProvider() const -> QString;
    void setTmpProvider(const QString&);

    Q_SIGNAL void typeNameChanged();
    Q_SIGNAL void tmpProviderChanged();

private:
    QString m_tmp_provider;
};
} // namespace qcm