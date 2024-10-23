#include "service_qml_ncm/model.h"

#include "service_qml_ncm/model/album_detail.h"
#include "service_qml_ncm/model/album_detail_dynamic.h"
#include "service_qml_ncm/model/album_sub.h"
#include "service_qml_ncm/model/album_sublist.h"
#include "service_qml_ncm/model/artist.h"
#include "service_qml_ncm/model/artist_albums.h"
#include "service_qml_ncm/model/artist_sub.h"
#include "service_qml_ncm/model/artist_sublist.h"
#include "service_qml_ncm/model/cloudsearch.h"
#include "service_qml_ncm/model/djradio_detail.h"
#include "service_qml_ncm/model/djradio_sub.h"
#include "service_qml_ncm/model/djradio_sublist.h"
#include "service_qml_ncm/model/djradio_program.h"
#include "service_qml_ncm/model/login.h"
#include "service_qml_ncm/model/logout.h"
#include "service_qml_ncm/model/playlist_catalogue.h"
#include "service_qml_ncm/model/playlist_detail.h"
#include "service_qml_ncm/model/playlist_detail_dynamic.h"
#include "service_qml_ncm/model/playlist_list.h"
#include "service_qml_ncm/model/playlist_subscribe.h"
#include "service_qml_ncm/model/playlist_create.h"
#include "service_qml_ncm/model/playlist_delete.h"
#include "service_qml_ncm/model/playlist_tracks.h"
#include "service_qml_ncm/model/recommend_songs.h"
#include "service_qml_ncm/model/recommend_resource.h"
#include "service_qml_ncm/model/song_url.h"
#include "service_qml_ncm/model/song_lyric.h"
#include "service_qml_ncm/model/user_playlist.h"
#include "service_qml_ncm/model/user_cloud.h"
#include "service_qml_ncm/model/qrcode_login.h"
#include "service_qml_ncm/model/qrcode_unikey.h"
#include "service_qml_ncm/model/radio_like.h"
#include "service_qml_ncm/model/song_like.h"

#include "service_qml_ncm/model/play_record.h"

#include "service_qml_ncm/api/user_account.h"

#include "qcm_interface/model/user_account.h"

#include "core/qlist_helper.h"
#include "core/strv_helper.h"

using namespace qcm;

namespace ncm
{

auto to_ncm_id(model::IdType t, i64 id) -> qcm::model::ItemId {
    return to_ncm_id(t, std::to_string(id));
}
auto to_ncm_id(model::IdType t, std::string_view id) -> qcm::model::ItemId {
    auto type_str = convert_from<std::string>(t);

    if (type_str.empty()) {
        _assert_msg_rel_(false, "unknown id type: {}", (int)t);
        return {};
    }

    ItemId out { provider, type_str, id };

    out.set_validator([](const auto& id) -> bool {
        return ! id.type().isEmpty() && id.id() != "0";
    });
    return out;
}

auto ncm_id_type(const ItemId& id) -> std::optional<model::IdType> {
    auto& t = id.type();
    if (t == "album")
        return model::IdType::Album;
    else if (t == "artist")
        return model::IdType::Artist;
    else if (t == "user")
        return model::IdType::User;
    else if (t == "program")
        return model::IdType::Program;
    else if (t == "djradio")
        return model::IdType::Djradio;
    else if (t == "song")
        return model::IdType::Song;
    else if (t == "comment")
        return model::IdType::Comment;
    else if (t == "playlist")
        return model::IdType::Playlist;
    else if (t == "special")
        return model::IdType::Special;
    else {
        //_assert_msg_rel_(false, "unknown id type: {}", t);
        return std::nullopt;
    }
}

auto to_ncm_id(const ItemId& id) -> model::IdTypes::append<std::monostate>::to<std::variant> {
    model::IdTypes::append<std::monostate>::to<std::variant> out { std::monostate {} };

    auto idx = ncm_id_type(id);
    do {
        if (! idx) break;
        model::IdTypes::runtime_select((u32)idx.value(), [&out, &id]<usize Idx, typename T>() {
            out = convert_from<T>(id);
        });
    } while (false);

    return out;
}

} // namespace ncm

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
    convert(out.subscribed, in.subscribed.value_or(false));
    convert(out.userId, in.userId);
};

IMPL_CONVERT(QDateTime, ncm::model::Time) { out = QDateTime::fromMSecsSinceEpoch(in.milliseconds); }

IMPL_CONVERT(qcm::model::Artist, ncm::model::Artist) {
    convert(out.id, in.id);
    convert(out.name, in.name);
    convert(out.picUrl, in.picUrl);
    convert(out.description, in.briefDesc.value_or(""));
    convert(out.musicCount, in.musicSize);
    convert(out.albumCount, in.albumSize);
    // convert(out.followed, in.followed);
}

#define X(prop, in) out.set_##prop(convert_from<std::remove_cvref_t<decltype(out.prop())>>(in))

IMPL_CONVERT(qcm::oper::ArtistOper, ncm::model::Artist) {
    X(id, in.id);
    X(name, in.name);
    X(picUrl, in.picUrl);
    X(description, in.briefDesc.value_or(""));
    X(musicCount, in.musicSize);
    X(albumCount, in.albumSize);
    // convert(out.followed, in.followed);
}

IMPL_CONVERT(qcm::oper::ArtistOper, ncm::model::ArtistSublistItem) {
    X(id, in.id);
    X(name, in.name);
    X(picUrl, in.picUrl);
    X(albumCount, in.albumSize);
}

IMPL_CONVERT(qcm::model::Artist, ncm::model::Song::Ar) {
    convert(out.id, in.id);
    convert(out.name, in.name.value_or(""));
    convert(out.alias, helper::value_or_default(in.alia));
}

IMPL_CONVERT(qcm::oper::ArtistOper, ncm::model::Song::Ar) {
    X(id, in.id);
    X(name, in.name.value_or(""));
    X(alias, helper::value_or_default(in.alia));
}

IMPL_CONVERT(qcm::model::Album, ncm::model::Album) {
    // convert(out.id, in.id);
    // convert(out.name, in.name);
    // convert(out.picUrl, in.picUrl);
    // convert(out.artists, in.artists);
    // convert(out.publishTime, in.publishTime);
    // convert(out.trackCount, std::max(in.size, (i64)in.songs.size()));
}

IMPL_CONVERT(qcm::oper::AlbumOper, ncm::model::Album) {
    X(id, in.id);
    X(name, in.name);
    X(picUrl, in.picUrl);
    X(publishTime, in.publishTime);
    X(trackCount, std::max(in.size, (i64)in.songs.size()));
    X(company, in.company.value_or(""));
    X(description, in.briefDesc.value_or(""));
    // use subtype
    X(type, in.subType);
}

IMPL_CONVERT(qcm::oper::AlbumOper, ncm::model::AlbumSublistItem) {
    X(id, in.id);
    X(name, in.name);
    X(picUrl, in.picUrl);
    X(trackCount, in.size);
    // X(artists, in.artists);
    // X(publishTime, in.publishTime);
}

IMPL_CONVERT(qcm::model::Song, ncm::model::Song) {
    convert(out.id, in.id);
    convert(out.name, in.name.value_or(""));
    // convert(out.album.id, in.al.id);
    // convert(out.album.name, in.al.name.value_or(""));
    // convert(out.album.picUrl, in.al.picUrl.value_or(""));
    convert(out.duration, in.dt);
    // convert(out.artists, in.ar);
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
        default: WARN_LOG("unknown fee: {}, {}", (i64)fee, in.name.value_or(""));
        }
        if (! tag.isEmpty()) out.tags.push_back(tag);
    }
}

IMPL_CONVERT(qcm::oper::SongOper, ncm::model::Song) {
    X(id, in.id);
    X(name, in.name.value_or(""));
    X(duration, in.dt);
    X(canPlay, (! in.privilege || in.privilege.value().st >= 0));
    X(trackNumber, in.no);
    X(albumId, in.al.id);
    out.set_popularity(in.pop);

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
        default: WARN_LOG("unknown fee: {}, {}", (i64)fee, in.name.value_or(""));
        }
        if (! tag.isEmpty()) {
            out.set_tags({ tag });
        }
    }
}

IMPL_CONVERT(qcm::model::Song, ncm::model::SongB) {
    convert(out.id, in.id);
    convert(out.name, in.name);
    // convert(out.album.id, in.album.id);
    // convert(out.album.name, in.album.name.value_or(""));
    // convert(out.album.picUrl, in.album.picUrl.value_or(""));
    convert(out.duration, in.duration);
    // convert(out.artists, in.artists);
    out.canPlay = true;
}

IMPL_CONVERT(qcm::oper::SongOper, ncm::model::SongB) {
    X(id, in.id);
    X(name, in.name);
    X(duration, in.duration);
    X(trackNumber, in.no);
    X(albumId, in.album.id);
    out.set_popularity(in.popularity);
    out.set_canPlay(true);
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

IMPL_CONVERT(qcm::model::Djradio, ncm::model::Djradio) {
    convert(out.id, in.id);
    convert(out.name, in.name);
    convert(out.picUrl, in.picUrl);
    convert(out.programCount, in.programCount);
}
IMPL_CONVERT(qcm::model::Djradio, ncm::model::DjradioB) {
    convert(out.id, in.id);
    convert(out.name, in.name);
    convert(out.picUrl, in.picUrl);
    convert(out.programCount, in.programCount);
}

IMPL_CONVERT(qcm::model::Program, ncm::model::Program) {
    convert(out.coverUrl, in.coverUrl);
    convert(out.duration, in.duration);
    convert(out.id, in.id);
    convert(out.name, in.name);
    convert(out.song, in.mainSong);
    convert(out.createTime, in.createTime);
    convert(out.serialNum, in.serialNum);
    if (in.mainSong.album.picId.value_or(1) == 0) {
        // convert(out.song.album.picUrl, in.coverUrl);
    }
}