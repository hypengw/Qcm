#pragma once

#include <QQmlEngine>
#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/song_url.h"

#include "core/log.h"

namespace qcm
{

namespace model
{

class SongUrlItem {
    Q_GADGET
public:
    GADGET_PROPERTY_DEF(ItemId, itemId, id)
    GADGET_PROPERTY_DEF(QString, url, url)
    GADGET_PROPERTY_DEF(QString, md5, md5)
};
} // namespace model
} // namespace qcm
DEFINE_CONVERT(qcm::model::SongUrlItem, ncm::model::SongUrl) {
    convert(out.id, in.id);
    convert(out.url, in.url);
    convert(out.md5, in.md5.value_or(""));
}

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

    FORWARD_PROPERTY(std::vector<model::ItemId>, ids, ids)
    FORWARD_PROPERTY_DECLARE(Level, level, level)
};
} // namespace qcm

DEFINE_CONVERT(qcm::SongUrlQuerier::Level, ncm::params::SongUrl::Level) {
    out = static_cast<qcm::SongUrlQuerier::Level>(in);
}

DEFINE_CONVERT(ncm::params::SongUrl::Level, qcm::SongUrlQuerier::Level) {
    out = static_cast<ncm::params::SongUrl::Level>(in);
}
namespace qcm
{
FORWARD_PROPERTY_IMPL(SongUrlQuerier, SongUrlQuerier::Level, level, level)
}
