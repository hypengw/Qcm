#include "Qcm/model.h"
#include "Qcm/model/album_detail.h"
#include "Qcm/model/album_detail_dynamic.h"
#include "Qcm/model/album_sub.h"
#include "Qcm/model/album_sublist.h"
#include "Qcm/model/artist.h"
#include "Qcm/model/artist_albums.h"
#include "Qcm/model/artist_sublist.h"
#include "Qcm/model/cloudsearch.h"
#include "Qcm/model/login.h"
#include "Qcm/model/playlist_catalogue.h"
#include "Qcm/model/playlist_detail.h"
#include "Qcm/model/playlist_detail_dynamic.h"
#include "Qcm/model/playlist_list.h"
#include "Qcm/model/playlist_subscribe.h"
#include "Qcm/model/recommend_songs.h"
#include "Qcm/model/recommend_resource.h"
#include "Qcm/model/song_url.h"
#include "Qcm/model/song_lyric.h"
#include "Qcm/model/user_playlist.h"
#include "Qcm/model/user_account.h"
#include "Qcm/model/qrcode_login.h"
#include "Qcm/model/qrcode_unikey.h"
#include "Qcm/model/radio_like.h"
#include "Qcm/model/song_like.h"

#include "core/qlist_helper.h"

using namespace qcm;

qcm::model::Playlist
To<qcm::model::Playlist>::From<ncm::model::Playlist>::from(const ncm::model::Playlist& in) {
    qcm::model::Playlist o;
    CONVERT_PROPERTY(o.id, in.id);
    CONVERT_PROPERTY(o.name, in.name);
    CONVERT_PROPERTY(o.picUrl, in.coverImgUrl);
    CONVERT_PROPERTY(o.description, in.description.value_or(""));
    if (in.updateTime) {
        CONVERT_PROPERTY(o.updateTime, in.updateTime.value());
    }
    CONVERT_PROPERTY(o.playCount, in.playCount);
    return o;
};

QDateTime To<QDateTime>::From<ncm::model::Time>::from(const ncm::model::Time& t) {
    return QDateTime::fromMSecsSinceEpoch(t.milliseconds);
}

qcm::model::Artist To<qcm::model::Artist>::from(const ncm::model::Artist& in) {
    qcm::model::Artist o;
    CONVERT_PROPERTY(o.id, in.id);
    CONVERT_PROPERTY(o.name, in.name);
    CONVERT_PROPERTY(o.picUrl, in.picUrl);
    CONVERT_PROPERTY(o.briefDesc, in.briefDesc.value_or(""));
    CONVERT_PROPERTY(o.musicSize, in.musicSize);
    CONVERT_PROPERTY(o.albumSize, in.albumSize);
    return o;
}
qcm::model::Artist To<qcm::model::Artist>::from(const ncm::model::Song::Ar& in) {
    qcm::model::Artist o;
    CONVERT_PROPERTY(o.id, in.id);
    CONVERT_PROPERTY(o.name, in.name);
    CONVERT_PROPERTY(o.alias, in.alia);
    return o;
}

qcm::model::Album To<qcm::model::Album>::from(const ncm::model::Album& in) {
    qcm::model::Album o;
    CONVERT_PROPERTY(o.id, in.id);
    CONVERT_PROPERTY(o.name, in.name);
    CONVERT_PROPERTY(o.picUrl, in.picUrl);
    CONVERT_PROPERTY(o.artists, in.artists);
    CONVERT_PROPERTY(o.publishTime, in.publishTime);
    CONVERT_PROPERTY(o.trackCount, std::max(in.size, (i64)in.songs.size()));
    return o;
}

model::Song To<model::Song>::from(const ncm::model::Song& in) {
    model::Song o;
    CONVERT_PROPERTY(o.id, in.id);
    CONVERT_PROPERTY(o.name, in.name);
    CONVERT_PROPERTY(o.album.id, in.al.id);
    CONVERT_PROPERTY(o.album.name, in.al.name);
    CONVERT_PROPERTY(o.album.picUrl, in.al.picUrl);
    CONVERT_PROPERTY(o.duration, in.dt);
    CONVERT_PROPERTY(o.artists, in.ar);
    CONVERT_PROPERTY(o.canPlay, (! in.privilege || in.privilege.value().st >= 0));

    if (in.privilege) {
        QString tag;
        auto    fee = in.privilege.value().fee;
        switch (fee) {
            using enum ncm::model::SongFee;
        case Vip: tag = "vip"; break;
        case OnlyOnlineWithPaid:
        case OnlyDownloadWithPaid: tag = "pay"; break;
        case DigitalAlbum: tag = "dg"; break;
        case Free:
        case Free128k: break;
        default: WARN_LOG("unknown fee: {}, {}", (i64)fee, in.name);
        }
        if (! tag.isEmpty()) o.tags.push_back(tag);
    }
    return o;
}
