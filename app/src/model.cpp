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

IMPL_CONVERT(qcm::model::Playlist, ncm::model::Playlist) {
    convert(out.id, in.id);
    convert(out.name, in.name);
    convert(out.picUrl, in.coverImgUrl);
    convert(out.description, in.description.value_or(""));
    if (in.updateTime) {
        convert(out.updateTime, in.updateTime.value());
    }
    convert(out.playCount, in.playCount);
    convert(out.trackCount, in.trackCount);
};

IMPL_CONVERT(QDateTime, ncm::model::Time) { out = QDateTime::fromMSecsSinceEpoch(in.milliseconds); }

IMPL_CONVERT(qcm::model::Artist, ncm::model::Artist) {
    convert(out.id, in.id);
    convert(out.name, in.name);
    convert(out.picUrl, in.picUrl);
    convert(out.briefDesc, in.briefDesc.value_or(""));
    convert(out.musicSize, in.musicSize);
    convert(out.albumSize, in.albumSize);
}

IMPL_CONVERT(qcm::model::Artist, ncm::model::Song::Ar) {
    convert(out.id, in.id);
    convert(out.name, in.name);
    convert(out.alias, in.alia);
}

IMPL_CONVERT(qcm::model::Album, ncm::model::Album) {
    convert(out.id, in.id);
    convert(out.name, in.name);
    convert(out.picUrl, in.picUrl);
    convert(out.artists, in.artists);
    convert(out.publishTime, in.publishTime);
    convert(out.trackCount, std::max(in.size, (i64)in.songs.size()));
}

IMPL_CONVERT(qcm::model::Song, ncm::model::Song) {
    convert(out.id, in.id);
    convert(out.name, in.name);
    convert(out.album.id, in.al.id);
    convert(out.album.name, in.al.name);
    convert(out.album.picUrl, in.al.picUrl);
    convert(out.duration, in.dt);
    convert(out.artists, in.ar);
    convert(out.canPlay, (! in.privilege || in.privilege.value().st >= 0));

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
        if (! tag.isEmpty()) out.tags.push_back(tag);
    }
}

IMPL_CONVERT(qcm::model::User, ncm::model::User) {
    convert(out.id, in.userId);
    convert(out.name, in.nickname);
    convert(out.picUrl, in.avatarUrl);
}

IMPL_CONVERT(qcm::model::Comment, ncm::model::Comment) {
    convert(out.content, in.content);
    convert(out.id, in.commentId);
    convert(out.liked, in.liked);
    convert(out.user, in.user);
    convert(out.time, in.time);
};