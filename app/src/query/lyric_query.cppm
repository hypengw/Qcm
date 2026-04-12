module;
#include "QExtra/macro_qt.hpp"

#ifdef Q_MOC_RUN
#    include "Qcm/query/lyric_query.moc"
#endif

export module qcm:query.lyric;
export import :query.query;
export import :model.list_models;
export import :model.lyric;
export import :msg;

namespace qcm
{

export class LyricQuery : public Query, public QueryExtra<LyricModel, LyricQuery> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
public:
    LyricQuery(QObject* parent = nullptr);
    void reload() override;

    auto itemId() const -> model::ItemId;
    void setItemId(model::ItemId);

    Q_SIGNAL void itemIdChanged();

private:
    model::ItemId m_item_id;
};

} // namespace qcm
