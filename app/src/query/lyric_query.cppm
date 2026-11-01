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

    Q_PROPERTY(qcm::msg::id::SongId songId READ songId WRITE setSongId NOTIFY songIdChanged)
public:
    LyricQuery(QObject* parent = nullptr);
    void reload() override;

    auto songId() const -> msg::id::SongId;
    void setSongId(msg::id::SongId);

    Q_SIGNAL void songIdChanged();

private:
    msg::id::SongId m_song_id;
};

} // namespace qcm
