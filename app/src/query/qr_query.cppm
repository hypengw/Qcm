module;
#include "Qcm/macro_qt.hpp"

#ifdef Q_MOC_RUN
#    include "Qcm/query/qr_query.moc"
#endif

export module qcm:query.qr;
export import :query.query;
export import :msg;

namespace qcm
{

export class QrAuthUrlQuery : public Query, public QueryExtra<msg::QrAuthUrlRsp, QrAuthUrlQuery> {
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
