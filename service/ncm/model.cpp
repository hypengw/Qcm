#include "ncm/model.h"
#include "json_helper/helper.inl"

#include "ncm/api/album_detail.h"
#include "ncm/api/album_detail_dynamic.h"
#include "ncm/api/album_sub.h"
#include "ncm/api/album_sublist.h"
#include "ncm/api/artist.h"
#include "ncm/api/artist_albums.h"
#include "ncm/api/artist_sublist.h"
#include "ncm/api/artist_sub.h"
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
#include "ncm/api/playlist_tracks.h"
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

#undef X

JSON_DEFINE_IMPL(Song::Ar, id, name, alia, picId);
JSON_DEFINE_IMPL(Song::Al, id, name, picUrl, picId);
JSON_DEFINE_IMPL(Song::Quality, br, size);
JSON_DEFINE_IMPL(Song::Privilege, downloadMaxBrLevel, playMaxBrLevel, downloadMaxbr, maxBrLevel,
                 playMaxbr, preSell, plLevel, flLevel, dlLevel, toast, payed, maxbr, subp, flag, sp,
                 pl, fl, dl, cs, fee, st, id, cp);

JSON_DEFINE_IMPL(Song, ar, al, st, rtype, pst, alia, pop, rt, mst, cp, cf, dt, ftype, no, fee, mv,
                 t, v, h, m, l, sq, hr, cd, name, id, privilege);
JSON_DEFINE_IMPL(SongB, name, ftype, album, artists, commentThreadId, copyright, copyrightId, disc,
                 duration, fee, hearTime, id, status, starred, score, popularity, playedNum)

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

JSON_DEFINE_IMPL(AlbumSublistItem, subTime, size, artists, id, name, picUrl, alias, transNames);

JSON_DEFINE_IMPL(ArtistSublistItem, mvSize, info, albumSize, trans, img1v1Url, picUrl, alias, id,
                 name);

JSON_DEFINE_IMPL(PlaylistCatalogue, resourceCount, resourceType, category, activity, imgId, hot,
                 type, name);
JSON_DEFINE_IMPL(Playlist, id, commentCount, specialType, shareCount, tracks, status, tags,
                 commentThreadId, updateTime, subscribed, name, coverImgUrl, playCount, description,
                 createTime, userId, trackCount);

JSON_DEFINE_IMPL(RecommendResourceItem, copywriter, playcount, picUrl, type, name, id, trackCount,
                 createTime);

JSON_DEFINE_IMPL(SongUrl, id, level, encodeType, size, br, fee, md5, url, time);
JSON_DEFINE_IMPL(SongLyricItem, version, lyric);

JSON_DEFINE_WITH_DEFAULT_IMPL(UserAccountProfile, defaultAvatar, accountStatus, accountType,
                              authority, avatarUrl, nickname, userType, vipType, gender,
                              description, createTime, userId, detailDescription, backgroundUrl);

JSON_DEFINE_IMPL(UserPlaylistItem, tags, description, commentThreadId, id, subscribedCount,
                 coverImgUrl, updateTime, trackCount, subscribed, createTime, playCount, userId,
                 name);

JSON_DEFINE_IMPL(DjradioDetail, name, category, categoryId, secondCategory, secondCategoryId,
                 commentCount, likedCount, playCount, programCount, shareCount,
                 lastProgramCreateTime, lastProgramId, subCount, desc, dynamic, feeScope, id,
                 original, picId, picUrl, privacy, radioFeeType, subed, createTime)

JSON_DEFINE_IMPL(Program, programFeeType, privacy, auditDisPlayStatus, auditStatus, pubStatus,
                 blurCoverUrl, coverId, coverUrl, buyed, canReward, categoryId, secondCategoryId,
                 secondCategoryName, createTime, scheduledPublishTime, commentCount,
                 commentThreadId, auditDisPlayStatus, id, mainSong, existLyric, duration, serialNum,
                 subscribed, score, name)

JSON_DEFINE_IMPL(UserCloudItem, fileName, fileSize, simpleSong, songName, songId, addTime, album,
                 artist, bitrate, cover, coverId, songId, lyricId, version)

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

JSON_DEFINE_IMPL(RecommendSongs::Data, dailySongs);
JSON_DEFINE_IMPL(RecommendSongs, code, data);

JSON_DEFINE_IMPL(RecommendResource, code, recommend);

JSON_DEFINE_IMPL(SongUrl, code, data);
JSON_DEFINE_IMPL(SongLyric, code, lrc, romalrc, tlyric, klyric);
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
JSON_DEFINE_IMPL(PlaylistTracks, code);

JSON_DEFINE_IMPL(UploadCloudInfo, code, songId);
JSON_DEFINE_IMPL(CloudUploadCheck, code, songId, needUpload);
JSON_DEFINE_IMPL(CloudPub, code);
JSON_DEFINE_IMPL(NosTokenAlloc::Result_, objectKey, token, resourceId);
JSON_DEFINE_IMPL(NosTokenAlloc, code, result);
JSON_DEFINE_IMPL(UploadAddr, lbs, upload);
JSON_DEFINE_IMPL(Upload, requestId, offset);

} // namespace api_model

} // namespace ncm

using namespace ncm;

JSON_GET_IMPL(api_model::ApiError);

JSON_GET_IMPL(api_model::AlbumDetail);
JSON_GET_IMPL(api_model::AlbumDetailDynamic);
JSON_GET_IMPL(api_model::AlbumSub);
JSON_GET_IMPL(api_model::AlbumSublist);
JSON_GET_IMPL(api_model::Artist);
JSON_GET_IMPL(api_model::ArtistAlbums);
JSON_GET_IMPL(api_model::ArtistSub);
JSON_GET_IMPL(api_model::ArtistSublist);
JSON_GET_IMPL(api_model::CloudSearch);
JSON_GET_IMPL(api_model::Comments);
JSON_GET_IMPL(api_model::Login);
JSON_GET_IMPL(api_model::Logout);
JSON_GET_IMPL(api_model::PlaylistCatalogue);
JSON_GET_IMPL(api_model::PlaylistDetail);
JSON_GET_IMPL(api_model::PlaylistDetailDynamic);
JSON_GET_IMPL(api_model::PlaylistList);
JSON_GET_IMPL(api_model::PlaylistSubscribe);
JSON_GET_IMPL(api_model::PlaylistCreate);
JSON_GET_IMPL(api_model::PlaylistDelete);
JSON_GET_IMPL(api_model::PlaylistTracks);
JSON_GET_IMPL(api_model::RecommendSongs);
JSON_GET_IMPL(api_model::RecommendResource);
JSON_GET_IMPL(api_model::SongUrl);
JSON_GET_IMPL(api_model::SongLyric);
JSON_GET_IMPL(api_model::UserAccount);
JSON_GET_IMPL(api_model::UserPlaylist);
JSON_GET_IMPL(api_model::QrcodeUnikey);
JSON_GET_IMPL(api_model::QrcodeLogin);
JSON_GET_IMPL(api_model::RadioLike);
JSON_GET_IMPL(api_model::SongLike);
JSON_GET_IMPL(api_model::DjradioSublist);
JSON_GET_IMPL(api_model::DjradioDetail);
JSON_GET_IMPL(api_model::DjradioProgram);
JSON_GET_IMPL(api_model::DjradioSub);
JSON_GET_IMPL(api_model::UserCloud);
JSON_GET_IMPL(api_model::UploadCloudInfo);
JSON_GET_IMPL(api_model::CloudUploadCheck);
JSON_GET_IMPL(api_model::CloudPub);
JSON_GET_IMPL(api_model::NosTokenAlloc);
JSON_GET_IMPL(api_model::UploadAddr);
JSON_GET_IMPL(api_model::Upload);

JSON_GET_IMPL(model::Album);
JSON_GET_IMPL(model::Song);

Params api::FeedbackWeblog::body() const {
    Params           p;
    qcm::json::njson j;
    j["action"] = "play";
    {
        qcm::json::njson j_;
        j_["download"] = 0;
        j_["end"]      = "playend";
        j_["id"]       = input.id;
        j_["sourceId"] = input.sourceId;
        j_["time"]     = input.time.milliseconds;
        j_["type"]     = "song";
        j_["wifi"]     = 0;
        j_["source"]   = "list";

        j["json"] = j_;
    }

    p["logs"] = j.dump();
    return p;
}