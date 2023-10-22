#pragma once

#include <QQmlEngine>
#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/song_url.h"

#include "core/log.h"

namespace qcm
{

namespace model
{

class SongUrlItem {
    Q_GADGET
public:
    GATGET_PROPERTY(SongId, itemId, id)
    GATGET_PROPERTY(QString, url, url)
    GATGET_PROPERTY(QString, md5, md5)
};
} // namespace model
} // namespace qcm
template<>
struct To<qcm::model::SongUrlItem> {
    static auto from(const ncm::model::SongUrl& in) {
        qcm::model::SongUrlItem o;
        convert(o.id, in.id);
        convert(o.url, in.url);
        convert(o.md5, in.md5);
        return o;
    }
};

namespace qcm
{
namespace model
{
class SongUrl : public QObject {
    Q_OBJECT
public:
    SongUrl(QObject* parent = nullptr): QObject(parent) {}

    Q_PROPERTY(std::vector<SongUrlItem> songs READ songs NOTIFY songsChanged)

    using out_type = ncm::api_model::SongUrl;

    void handle_output(const out_type& in, const auto&) {
        auto& o = m_songs;
        convert(o, in.data);
        emit songsChanged();
    }

    const std::vector<SongUrlItem>& songs() const { return m_songs; }

signals:
    void songsChanged();

private:
    std::vector<SongUrlItem> m_songs;
};
static_assert(modelable<SongUrl, ncm::api::SongUrl>);

} // namespace model

using SongUrlQuerier_base = ApiQuerier<ncm::api::SongUrl, model::SongUrl>;
class SongUrlQuerier : public SongUrlQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    enum Level
    {
        LevelStandard = 0,
        LevelHigher,
        LevelExhigh,
        LevelLossless,
        LevelHires
    };
    Q_ENUM(Level)

public:
    SongUrlQuerier(QObject* parent = nullptr): SongUrlQuerier_base(parent) {}

    FORWARD_PROPERTY(std::vector<model::SongId>, ids, ids)
    FORWARD_PROPERTY_DECLARE(Level, level, level)
};
} // namespace qcm

template<>
struct To<qcm::SongUrlQuerier::Level> {
    static auto from(ncm::params::SongUrl::Level l) {
        return static_cast<qcm::SongUrlQuerier::Level>(l);
    }
};
template<>
struct To<ncm::params::SongUrl::Level> {
    static auto from(qcm::SongUrlQuerier::Level l) {
        return static_cast<ncm::params::SongUrl::Level>(l);
    }
};

namespace qcm
{
FORWARD_PROPERTY_IMPL(SongUrlQuerier, SongUrlQuerier::Level, level, level)
}
