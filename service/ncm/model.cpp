#include "ncm/model.h"
#include "json_helper/helper.inl"

#include "ncm/api/v1_album.h"
#include "ncm/api/album_detail_dynamic.h"
#include "ncm/api/album_sub.h"
#include "ncm/api/album_sublist.h"
#include "ncm/api/artist.h"
#include "ncm/api/artist_albums.h"
#include "ncm/api/artist_sublist.h"
#include "ncm/api/artist_sub.h"
#include "ncm/api/artist_songs.h"
#include "ncm/api/cloudsearch.h"
#include "ncm/api/comments.h"
#include "ncm/api/djradio_detail.h"
#include "ncm/api/djradio_sub.h"
#include "ncm/api/djradio_sublist.h"
#include "ncm/api/djradio_program.h"
#include "ncm/api/login.h"
#include "ncm/api/logout.h"
#include "ncm/api/playlist_catalogue.h"
#include "ncm/api/playlist_detail.h"
#include "ncm/api/playlist_detail_dynamic.h"
#include "ncm/api/playlist_list.h"
#include "ncm/api/playlist_subscribe.h"
#include "ncm/api/playlist_create.h"
#include "ncm/api/playlist_delete.h"
#include "ncm/api/playlist_manipulate_tracks.h"
#include "ncm/api/playlist_update_name.h"
#include "ncm/api/recommend_songs.h"
#include "ncm/api/recommend_resource.h"
#include "ncm/api/song_url.h"
#include "ncm/api/song_lyric.h"
#include "ncm/api/user_account.h"
#include "ncm/api/user_playlist.h"
#include "ncm/api/user_cloud.h"
#include "ncm/api/qrcode_login.h"
#include "ncm/api/qrcode_unikey.h"
#include "ncm/api/radio_like.h"
#include "ncm/api/song_like.h"
#include "ncm/api/feedback_weblog.h"

#include "ncm/api/upload_cloud_info.h"
#include "ncm/api/cloud_upload_check.h"
#include "ncm/api/cloud_pub.h"
#include "ncm/api/nos_token_alloc.h"
#include "ncm/api/upload_addr.h"
#include "ncm/api/upload.h"
#include "ncm/api/song_detail.h"
#include "ncm/api/v1_radio_get.h"

#include "ncm/api/play_record.h"

namespace ncm
{
namespace model
{

void to_json(nlohmann::json& j, const Time& t) {
    j = std::chrono::duration_cast<std::chrono::milliseconds>(t.point.time_since_epoch()).count();
}
void from_json(const nlohmann::json& j, Time& t) {
    t.milliseconds = j.get<i64>();
    t.point        = Time::time_point { std::chrono::duration<i64, std::milli> { t.milliseconds } };
}

#define X(T)                                                  \
    void to_json(nlohmann::json& j, const T& t) { j = t.id; } \
    void from_json(const nlohmann::json& j, T& t) { t.id = j.get<decltype(t.id)>(); }

X(AlbumId)
X(SongId)
X(ProgramId)
X(ArtistId)
X(PlaylistId)
X(UserId)
X(CommentId)
X(DjradioId)
X(SpecialId)

#undef X

JSON_DEFINE_IMPL(Song::Ar, id, name, alia, picId);
JSON_DEFINE_IMPL(Song::Al, id, name, picUrl, picId);
JSON_DEFINE_IMPL(Song::Quality, br, size);
JSON_DEFINE_IMPL(Song::Privilege, downloadMaxBrLevel, playMaxBrLevel, downloadMaxbr, maxBrLevel,
                 playMaxbr, preSell, plLevel, flLevel, dlLevel, toast, payed, maxbr, subp, flag, sp,
                 pl, fl, dl, cs, fee, st, id, cp);

JSON_DEFINE_IMPL(SongB, name, no, ftype, album, artists, commentThreadId, copyright, copyrightId,
                 disc, duration, fee, hearTime, id, status, starred, score, popularity, playedNum)

struct Song_ : Song {};
JSON_DEFINE_IMPL(Song_, ar, al, st, rtype, pst, alia, pop, rt, mst, cp, cf, dt, ftype, no, fee, mv,
                 t, v, h, m, l, sq, hr, cd, name, id, privilege);
void to_json(nlohmann::json& j, const Song& s) {
    auto w = static_cast<const Song_*>(&s);
    to_json(j, *w);
}
void from_json(const nlohmann::json& j, Song& s) {
    if (j.contains("al")) {
        auto w = static_cast<Song_*>(&s);
        from_json(j, *w);
    } else {
        SongB sb;
        from_json(j, sb);
        s.al    = sb.album;
        s.ar    = sb.artists;
        s.id    = sb.id;
        s.name  = sb.name;
        s.fee   = sb.fee;
        s.dt    = sb.duration;
        s.ftype = sb.ftype;
    }
}

JSON_DEFINE_WITH_DEFAULT_IMPL(Artist, followed, alias, trans, musicSize, albumSize, briefDesc,
                              picUrl, img1v1Url, name, id);

JSON_DEFINE_WITH_DEFAULT_IMPL(Album, songs, paid, onSale, mark, companyId, blurPicUrl, alias,
                              artists, copyrightId, artist, briefDesc, publishTime, company, picUrl,
                              commentThreadId, description, tags, status, subType, name, id, type,
                              size, picId_str);

JSON_DEFINE_IMPL(User, userId, userType, vipType, avatarUrl, followed, nickname)

JSON_DEFINE_IMPL(Comment, user, richContent, commentId, content, liked, likedCount, owner, status,
                 time)

JSON_DEFINE_IMPL(Djradio, createTime, buyed, category, categoryId, secondCategory, desc, dynamic,
                 feeScope, finished, id, intervenePicUrl, name, originalPrice, picId, picUrl,
                 playCount, privacy, radioFeeType, programCount, lastProgramCreateTime,
                 lastProgramId)
JSON_DEFINE_IMPL(DjradioB, categoryId, id, name, picUrl, playCount, programCount, lastProgramId)

JSON_DEFINE_IMPL(AlbumSublistItem, subTime, size, artists, id, name, picUrl, alias, transNames);

JSON_DEFINE_IMPL(ArtistSublistItem, mvSize, info, albumSize, trans, img1v1Url, picUrl, alias, id,
                 name);

JSON_DEFINE_IMPL(PlaylistCatalogue, resourceCount, resourceType, category, activity, imgId, hot,
                 type, name);

JSON_DEFINE_IMPL(Playlist::TrackId, id)
JSON_DEFINE_IMPL(Playlist, id, commentCount, specialType, shareCount, tracks, trackIds, status,
                 tags, commentThreadId, updateTime, subscribed, name, coverImgUrl, playCount,
                 description, createTime, userId, trackCount);

JSON_DEFINE_IMPL(RecommendResourceItem, copywriter, playcount, picUrl, type, name, id, trackCount,
                 createTime);

JSON_DEFINE_IMPL(SongUrl, id, level, encodeType, size, br, fee, md5, url, time);
JSON_DEFINE_IMPL(SongLyricItem, version, lyric);

JSON_DEFINE_WITH_DEFAULT_IMPL(UserAccountProfile, defaultAvatar, accountStatus, accountType,
                              authority, avatarUrl, nickname, userType, vipType, gender,
                              description, createTime, userId, detailDescription, backgroundUrl);

JSON_DEFINE_IMPL(UserPlaylistItem, tags, description, commentThreadId, id, subscribedCount,
                 coverImgUrl, updateTime, trackCount, subscribed, createTime, playCount, userId,
                 name, specialType);

JSON_DEFINE_IMPL(DjradioDetail, name, category, categoryId, secondCategory, secondCategoryId,
                 commentCount, likedCount, playCount, programCount, shareCount,
                 lastProgramCreateTime, lastProgramId, subCount, desc, dynamic, feeScope, id,
                 original, picId, picUrl, privacy, radioFeeType, subed, createTime)

JSON_DEFINE_IMPL(Program, programFeeType, privacy, auditDisPlayStatus, auditStatus, pubStatus,
                 blurCoverUrl, coverId, coverUrl, buyed, canReward, categoryId, secondCategoryId,
                 secondCategoryName, createTime, scheduledPublishTime, commentCount,
                 commentThreadId, auditDisPlayStatus, id, mainSong, existLyric, duration, serialNum,
                 subscribed, score, name, radio, channels)

// clang-format off
JSON_DEFINE_IMPL(Creator, 
    defaultAvatar,
    province,
    authStatus,
    followed,
    avatarUrl,
    accountStatus,
    gender,
    city,
    birthday,
    userId,
    userType,
    nickname,
    signature,
    description,
    detailDescription,
    avatarImgId,
    backgroundImgId,
    backgroundUrl,
    authority,
    mutual,
    djStatus,
    vipType,
    avatarImgIdStr,
    backgroundImgIdStr
);
// clang-format on

JSON_DEFINE_IMPL(UserCloudItem, fileName, fileSize, simpleSong, songName, songId, addTime, album,
                 artist, bitrate, cover, coverId, songId, lyricId, version)

JSON_DEFINE_IMPL(PlayRecordItem, playTime, banned, multiTerminalInfo, resourceId, resourceType);
JSON_DEFINE_IMPL(PlayRecordItem::MultiTerminalInfo, icon, os, osText);
JSON_DEFINE_IMPL(PlayRecordItem::Playlist, coverImgUrl, id, lastSong, name, creator);

} // namespace model

namespace api_model
{

JSON_DEFINE_WITH_DEFAULT_IMPL(ApiError, code, message, errMsg);

JSON_DEFINE_IMPL(AlbumDetail, code, songs, album);
JSON_DEFINE_IMPL(AlbumDetailDynamic, code, shareCount, subCount, subTime, onSale, isSub,
                 commentCount, likedCount);
JSON_DEFINE_IMPL(AlbumSub, code);
JSON_DEFINE_IMPL(AlbumSublist, code, data, count, hasMore);
JSON_DEFINE_IMPL(Artist, code, hotSongs, artist, more);
JSON_DEFINE_IMPL(ArtistAlbums, code, hotAlbums, more);
JSON_DEFINE_IMPL(ArtistSongs, code, songs, more, total);
JSON_DEFINE_IMPL(ArtistSub, code);
JSON_DEFINE_IMPL(ArtistSublist, code, data, count, hasMore);
JSON_DEFINE_IMPL(Login, code);
JSON_DEFINE_IMPL(Logout, code);
JSON_DEFINE_IMPL(PlaylistDetail, code, playlist, privileges);
JSON_DEFINE_IMPL(PlaylistDetailDynamic, code, bookedCount, subscribed, playCount, followed,
                 shareCount);

JSON_DEFINE_IMPL(PlaylistCatalogue, code, sub, all, categories);
JSON_DEFINE_IMPL(PlaylistList, code, playlists, total, more, cat);
JSON_DEFINE_IMPL(PlaylistSubscribe, code);
JSON_DEFINE_IMPL(PlaylistUpdateName, code);

JSON_DEFINE_IMPL(RecommendSongs::Data, dailySongs);
JSON_DEFINE_IMPL(RecommendSongs, code, data);

JSON_DEFINE_IMPL(RecommendResource, code, recommend);

JSON_DEFINE_IMPL(SongUrl, code, data);
JSON_DEFINE_IMPL(SongLyric, code, lrc, romalrc, tlyric, klyric);
JSON_DEFINE_IMPL(SongDetail, code, songs, privileges);
JSON_DEFINE_WITH_DEFAULT_IMPL(UserAccount, code, profile);
JSON_DEFINE_IMPL(UserPlaylist, playlist, more);

JSON_DEFINE_IMPL(QrcodeUnikey, code, unikey);
JSON_DEFINE_WITH_DEFAULT_IMPL(QrcodeLogin, code, message, nickname, avatarUrl);
JSON_DEFINE_IMPL(RadioLike, code);
JSON_DEFINE_IMPL(SongLike, code, ids, checkPoint);

JSON_DEFINE_IMPL(CloudSearch::SongResult, songCount, songs);
JSON_DEFINE_IMPL(CloudSearch::AlbumResult, albumCount, albums);
JSON_DEFINE_IMPL(CloudSearch::PlaylistResult, playlistCount, playlists);
JSON_DEFINE_IMPL(CloudSearch::ArtistResult, artistCount, artists);
JSON_DEFINE_IMPL(CloudSearch::DjradioResult, djRadiosCount, djRadios);
JSON_DEFINE_IMPL(CloudSearch, result);

JSON_DEFINE_IMPL(Comments, comments, hotComments, topComments, more, moreHot, total);

JSON_DEFINE_IMPL(DjradioSublist, djRadios, hasMore, count, time);
JSON_DEFINE_IMPL(DjradioDetail, data);

JSON_DEFINE_IMPL(DjradioProgram, programs, count, more);
JSON_DEFINE_IMPL(DjradioSub, code);

JSON_DEFINE_IMPL(UserCloud, data, count, hasMore, maxSize, size, upgradeSign);
JSON_DEFINE_IMPL(PlaylistCreate, id, playlist);
JSON_DEFINE_IMPL(PlaylistDelete, code);
JSON_DEFINE_IMPL(PlaylistManipulateTracks, code);

JSON_DEFINE_IMPL(UploadCloudInfo, code, songId);
JSON_DEFINE_IMPL(CloudUploadCheck, code, songId, needUpload);
JSON_DEFINE_IMPL(CloudPub, code);
JSON_DEFINE_IMPL(NosTokenAlloc::Result_, objectKey, token, resourceId);
JSON_DEFINE_IMPL(NosTokenAlloc, code, result);
JSON_DEFINE_IMPL(UploadAddr, lbs, upload);
JSON_DEFINE_IMPL(Upload, requestId, offset);
JSON_DEFINE_IMPL(FeedbackWeblog, code, data);

JSON_DEFINE_IMPL(PlayRecord, code, data);
JSON_DEFINE_IMPL(PlayRecord::Data, list, total);

JSON_DEFINE_IMPL(RadioGet, code, popAdjust, data);

} // namespace api_model

} // namespace ncm

JSON_GET_IMPL(ncm::api_model::ApiError);
JSON_GET_IMPL(ncm::api_model::AlbumDetail);
JSON_GET_IMPL(ncm::api_model::AlbumDetailDynamic);
JSON_GET_IMPL(ncm::api_model::AlbumSub);
JSON_GET_IMPL(ncm::api_model::AlbumSublist);
JSON_GET_IMPL(ncm::api_model::Artist);
JSON_GET_IMPL(ncm::api_model::ArtistAlbums);
JSON_GET_IMPL(ncm::api_model::ArtistSongs);
JSON_GET_IMPL(ncm::api_model::ArtistSub);
JSON_GET_IMPL(ncm::api_model::ArtistSublist);
JSON_GET_IMPL(ncm::api_model::CloudSearch);
JSON_GET_IMPL(ncm::api_model::Comments);
JSON_GET_IMPL(ncm::api_model::Login);
JSON_GET_IMPL(ncm::api_model::Logout);
JSON_GET_IMPL(ncm::api_model::PlaylistCatalogue);
JSON_GET_IMPL(ncm::api_model::PlaylistDetail);
JSON_GET_IMPL(ncm::api_model::PlaylistDetailDynamic);
JSON_GET_IMPL(ncm::api_model::PlaylistList);
JSON_GET_IMPL(ncm::api_model::PlaylistSubscribe);
JSON_GET_IMPL(ncm::api_model::PlaylistCreate);
JSON_GET_IMPL(ncm::api_model::PlaylistDelete);
JSON_GET_IMPL(ncm::api_model::PlaylistManipulateTracks);
JSON_GET_IMPL(ncm::api_model::PlaylistUpdateName);
JSON_GET_IMPL(ncm::api_model::RecommendSongs);
JSON_GET_IMPL(ncm::api_model::RecommendResource);
JSON_GET_IMPL(ncm::api_model::SongUrl);
JSON_GET_IMPL(ncm::api_model::SongLyric);
JSON_GET_IMPL(ncm::api_model::SongDetail);
JSON_GET_IMPL(ncm::api_model::UserAccount);
JSON_GET_IMPL(ncm::api_model::UserPlaylist);
JSON_GET_IMPL(ncm::api_model::QrcodeUnikey);
JSON_GET_IMPL(ncm::api_model::QrcodeLogin);
JSON_GET_IMPL(ncm::api_model::RadioLike);
JSON_GET_IMPL(ncm::api_model::SongLike);
JSON_GET_IMPL(ncm::api_model::DjradioSublist);
JSON_GET_IMPL(ncm::api_model::DjradioDetail);
JSON_GET_IMPL(ncm::api_model::DjradioProgram);
JSON_GET_IMPL(ncm::api_model::DjradioSub);
JSON_GET_IMPL(ncm::api_model::UserCloud);
JSON_GET_IMPL(ncm::api_model::UploadCloudInfo);
JSON_GET_IMPL(ncm::api_model::CloudUploadCheck);
JSON_GET_IMPL(ncm::api_model::CloudPub);
JSON_GET_IMPL(ncm::api_model::NosTokenAlloc);
JSON_GET_IMPL(ncm::api_model::UploadAddr);
JSON_GET_IMPL(ncm::api_model::Upload);
JSON_GET_IMPL(ncm::api_model::FeedbackWeblog);
JSON_GET_IMPL(ncm::api_model::RadioGet);

JSON_GET_IMPL(ncm::api_model::PlayRecord);

JSON_GET_IMPL(ncm::model::Album);
JSON_GET_IMPL(ncm::model::Song);

auto ncm::api::FeedbackWeblog::body() const -> Params {
    Params           p;
    qcm::json::njson j;
    j["action"] = convert_from<std::string>(input.act);
    {
        qcm::json::njson j_;

        j_["mainsite"] = input.mainsite;
        if (! input.alg.empty()) j_["alg"] = input.alg;

        bool is_end = input.act == params::FeedbackWeblog::Action::End;

        std::visit(overloaded {
                       [&j_](model::SongId id) {
                           j_["id"]   = id.as_i64();
                           j_["type"] = "song";
                       },
                       [&j_](model::ProgramId id) {
                           j_["id"]   = id.as_i64();
                           j_["type"] = "dj";
                       },
                   },
                   input.id);

        auto get_source = [](model::IdType t) -> std::string_view {
            switch (t) {
            case ncm::model::IdType::Playlist: return "list"sv;
            case ncm::model::IdType::Album: return "album"sv;
            case ncm::model::IdType::Djradio: return "djradio"sv;
            default: return "list"sv;
            }
        };

        std::visit(overloaded { [&j_](model::SpecialId id) {
                                   auto sid       = id.as_str();
                                   j_["sourceId"] = sid;
                                   j_["source"]   = j_["id"];
                                   j_["content"]  = "";
                               },
                                [&j_, &get_source](auto id) {
                                    auto sid       = id.as_str();
                                    j_["sourceId"] = sid;
                                    j_["source"]   = get_source(id.id_type);
                                    j_["content"] =
                                        sid.empty() ? std::string {} : fmt::format("id={}", sid);
                                },
                                [](std::monostate) {
                                } },
                   input.sourceId);
        if (is_end) {
            j_["wifi"]     = input.wifi;
            j_["download"] = input.download;
            j_["time"]     = input.time;
            j_["end"]      = input.end;
        } else {
            j_.erase("source");
            j_.erase("sourceId");
        }

        j["json"] = j_;
    }

    p["logs"] = fmt::format("[{}]", j.dump());
    return p;
}

// used by record api
IMPL_CONVERT(std::string, ncm::model::IdType) {
    switch (in) {
    case ncm::model::IdType::Album: out = "album"sv; break;
    case ncm::model::IdType::User: out = "user"sv; break;
    case ncm::model::IdType::Artist: out = "artist"sv; break;
    case ncm::model::IdType::Comment: out = "comment"sv; break;
    case ncm::model::IdType::Djradio: out = "radio"sv; break;
    case ncm::model::IdType::Song: out = "song"sv; break;
    case ncm::model::IdType::Program: out = "program"sv; break;
    case ncm::model::IdType::Playlist: out = "playlist"sv; break;
    case ncm::model::IdType::Special: out = "special"sv; break;
    default: {
        out = {};
    }
    }
}

namespace ncm::model
{

namespace
{
template<typename T>
struct get_model_type;
template<>
struct get_model_type<SongId> {
    using type = Song;
};
template<>
struct get_model_type<AlbumId> {
    using type = Album;
};
template<>
struct get_model_type<PlaylistId> {
    using type = PlayRecordItem::Playlist;
};
template<>
struct get_model_type<DjradioId> {
    using type = DjradioB;
};
template<typename T>
using get_model_type_t = typename get_model_type<T>::type;
} // namespace
} // namespace ncm::model
namespace ncm::api_model
{

auto PlayRecord::parse(std::span<const byte> bs, const params::PlayRecord& input)
    -> Result<PlayRecord> {
    return json::parse(convert_from<std::string_view>(bs))
        .map_error([](auto err) {
            return Error::push(err);
        })
        .and_then([&input](auto j) -> Result<PlayRecord> {
            if (auto err = check_api_error(*j)) {
                return nstd::unexpected(Error::push(err.value()));
            }
            Result<PlayRecord> out = json::get<PlayRecord>(*j, {}).map_error([](auto err) {
                return Error::push(err);
            });
            if (out) {
                json::njson& list = j->at("/data/list"_json_pointer);

                model::IdTypes::runtime_select(
                    (u32)input.type, [&out, &list]<usize I, typename T>() {
                        if constexpr (ycore::type_list<model::SongId,
                                                       model::AlbumId,
                                                       model::PlaylistId,
                                                       model::DjradioId>::contains<T>()) {
                            auto n = std::min(list.size(), out->data.list.size());
                            for (usize i = 0; i < n; i++) {
                                auto res = parse_no_apierr<model::get_model_type_t<T>>(
                                               list.at(i).at("data"))
                                               .map([&out, i](const auto& val) {
                                                   out->data.list[i].data = val;
                                               });
                                if (! res) {
                                    out = nstd::unexpected(res.error());
                                    break;
                                }
                            }
                        }
                    });
            }
            return out;
        });
}
} // namespace ncm::api_model

IMPL_CONVERT(std::string_view, ncm::params::RadioGet::Mode) {
    switch (in) {
    case in_type::AIDJ: out = "aidj"sv; break;
    case in_type::DEFAULT: out = "DEFAULT"sv; break;
    case in_type::EXPLORE: out = "EXPLORE"sv; break;
    case in_type::FAMILIAR: out = "FAMILIAR"sv; break;
    case in_type::SCENE_RCMD: out = "SCENE_RCMD"sv; break;
    }
}
IMPL_CONVERT(std::string_view, ncm::params::RadioGet::SubMode) {
    switch (in) {
    case in_type::EXERCISE: out = "EXERCISE"sv; break;
    case in_type::FOCUS: out = "FOCUS"sv; break;
    case in_type::NIGHT_EMO: out = "NIGHT_EMO"sv; break;
    default: {
    }
    }
}
