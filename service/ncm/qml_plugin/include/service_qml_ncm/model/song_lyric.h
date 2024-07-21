#pragma once

#include <QSet>
#include <QQmlEngine>
#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/song_lyric.h"

#include "core/log.h"

namespace qcm
{
namespace model
{
class SongLyric : public QObject {
    Q_OBJECT
public:
    SongLyric(QObject* parent = nullptr): QObject(parent) {}

    READ_PROPERTY(QString, lrc, m_lrc, infoChanged)
    READ_PROPERTY(QString, transLrc, m_trans_lrc, infoChanged)
    READ_PROPERTY(QString, romaLrc, m_roma_lrc, infoChanged)

    using out_type = ncm::api_model::SongLyric;
    void handle_output(const out_type& in, const auto&) {
        convert(m_lrc, in.lrc.lyric);
        m_trans_lrc.clear();
        if (in.tlyric) {
            convert(m_trans_lrc, in.tlyric->lyric);
        }
        m_roma_lrc.clear();
        if (in.romalrc) {
            convert(m_roma_lrc, in.romalrc->lyric);
        }
        emit infoChanged();
    }

signals:
    void infoChanged();
};
static_assert(modelable<SongLyric, ncm::api::SongLyric>);

} // namespace model

using SongLyricQuerier_base = ApiQuerier<ncm::api::SongLyric, model::SongLyric>;
class SongLyricQuerier : public SongLyricQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    SongLyricQuerier(QObject* parent = nullptr): SongLyricQuerier_base(parent) {}

    FORWARD_PROPERTY(model::ItemId, songId, id)
};
} // namespace qcm
