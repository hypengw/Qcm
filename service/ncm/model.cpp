#include "ncm/model.h"
#include "json_helper/helper.inl"

#include "ncm/api/album_detail.h"
#include "ncm/api/album_detail_dynamic.h"
#include "ncm/api/album_sub.h"
#include "ncm/api/album_sublist.h"
#include "ncm/api/artist.h"
#include "ncm/api/artist_sublist.h"
#include "ncm/api/login.h"
#include "ncm/api/playlist_detail.h"
#include "ncm/api/playlist_detail_dynamic.h"
#include "ncm/api/playlist_subscribe.h"
#include "ncm/api/song_url.h"
#include "ncm/api/user_account.h"
#include "ncm/api/user_playlist.h"
#include "ncm/api/qrcode_unikey.h"
#include "ncm/api/qrcode_login.h"

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

JSON_DEFINE_WITH_DEFAULT_IMPL(Song::Ar, id, name, alia);
JSON_DEFINE_WITH_DEFAULT_IMPL(Song::Al, id, name, picUrl);
JSON_DEFINE_IMPL(Song::Quality, br, size);

JSON_DEFINE_WITH_DEFAULT_IMPL(Song, ar, al, st, rtype, pst, alia, pop, rt, mst, cp, cf, dt, ftype,
                              no, fee, mv, t, v, h, m, l, sq, hr, cd, name, id);

JSON_DEFINE_WITH_DEFAULT_IMPL(Artist, followed, alias, trans, musicSize, albumSize, briefDesc,
                              picUrl, img1v1Url, name, id);

JSON_DEFINE_WITH_DEFAULT_IMPL(Album, songs, paid, onSale, mark, companyId, blurPicUrl, alias,
                              artists, copyrightId, artist, briefDesc, publishTime, company, picUrl,
                              commentThreadId, description, tags, status, subType, name, id, type,
                              size, picId_str);

JSON_DEFINE_IMPL(AlbumSublistItem, subTime, size, artists, id, name, picUrl, alias, transNames);

JSON_DEFINE_IMPL(ArtistSublistItem, mvSize, info, albumSize, trans, img1v1Url, picUrl, alias, id,
                 name);

JSON_DEFINE_IMPL(PlaylistDetail, id, commentCount, specialType, shareCount, tracks, status, tags,
                 commentThreadId, updateTime, subscribed, name, coverImgUrl, playCount, description,
                 createTime, userId, trackCount);

JSON_DEFINE_IMPL(SongUrl, id, level, encodeType, size, br, fee, md5, url, time);

JSON_DEFINE_WITH_DEFAULT_IMPL(UserAccountProfile, defaultAvatar, accountStatus, accountType,
                              authority, avatarUrl, nickname, userType, vipType, gender,
                              description, createTime, userId, detailDescription, backgroundUrl);

JSON_DEFINE_IMPL(UserPlaylistItem, tags, description, commentThreadId, id, subscribedCount,
                 coverImgUrl, updateTime, trackCount, subscribed, createTime, playCount, userId,
                 name);

} // namespace model

namespace api_model
{

JSON_DEFINE_WITH_DEFAULT_IMPL(ApiError, code, message);

JSON_DEFINE_IMPL(AlbumDetail, code, songs, album);
JSON_DEFINE_IMPL(AlbumDetailDynamic, code, shareCount, subCount, subTime, onSale, isSub,
                 commentCount, likedCount);
JSON_DEFINE_IMPL(AlbumSub, code);
JSON_DEFINE_IMPL(AlbumSublist, code, data, count, hasMore);
JSON_DEFINE_IMPL(Artist, code, hotSongs, artist, more);
JSON_DEFINE_IMPL(ArtistSublist, code, data, count, hasMore);
JSON_DEFINE_IMPL(Login, code);
JSON_DEFINE_IMPL(PlaylistDetail, code, playlist);
JSON_DEFINE_IMPL(PlaylistDetailDynamic, code, bookedCount, subscribed, playCount, followed,
                 shareCount);
JSON_DEFINE_IMPL(PlaylistSubscribe, code);

JSON_DEFINE_IMPL(SongUrl, code, data);
JSON_DEFINE_WITH_DEFAULT_IMPL(UserAccount, code, profile);
JSON_DEFINE_IMPL(UserPlaylist, code, playlist, more);

JSON_DEFINE_IMPL(QrcodeUnikey, code, unikey);
JSON_DEFINE_WITH_DEFAULT_IMPL(QrcodeLogin, code, message, nickname, avatarUrl);

} // namespace api_model

} // namespace ncm

using namespace ncm;


JSON_GET_IMPL(api_model::ApiError);

JSON_GET_IMPL(api_model::AlbumDetail);
JSON_GET_IMPL(api_model::AlbumDetailDynamic);
JSON_GET_IMPL(api_model::AlbumSub);
JSON_GET_IMPL(api_model::AlbumSublist);
JSON_GET_IMPL(api_model::Artist);
JSON_GET_IMPL(api_model::ArtistSublist);
JSON_GET_IMPL(api_model::Login);
JSON_GET_IMPL(api_model::PlaylistDetail);
JSON_GET_IMPL(api_model::PlaylistDetailDynamic);
JSON_GET_IMPL(api_model::PlaylistSubscribe);
JSON_GET_IMPL(api_model::SongUrl);
JSON_GET_IMPL(api_model::UserAccount);
JSON_GET_IMPL(api_model::UserPlaylist);
JSON_GET_IMPL(api_model::QrcodeUnikey);
JSON_GET_IMPL(api_model::QrcodeLogin);

JSON_GET_IMPL(model::Album);
JSON_GET_IMPL(model::Song);
