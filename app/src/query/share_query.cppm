module;
#include "QExtra/macro_qt.hpp"
#ifdef Q_MOC_RUN
#    include "Qcm/query/share_query.moc"
#endif
export module qcm:query.share;
export import :query.query;
export import :model.store_item;

namespace qcm
{
export class ShareLinkQuery : public Query,
                              public QueryExtra<msg::GetShareLinkRsp, ShareLinkQuery> {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged FINAL)
public:
    ShareLinkQuery(QObject* parent = nullptr);
    void          reload() override;
    model::ItemId itemId() const;
    void          setItemId(model::ItemId);
    Q_SIGNAL void itemIdChanged();

private:
    model::ItemId m_item_id;
};
} // namespace qcm
