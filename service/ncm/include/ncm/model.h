#pragma once

#include <chrono>

#include "json_helper/helper.h"
#include "ncm/type.h"

namespace ncm
{
namespace model
{

struct Time {
    using time_point = std::chrono::system_clock::time_point;
    i64        milliseconds;
    time_point point;
};

enum class SongFee
{
    Free                 = 0,
    Vip                  = 1,
    DigitalAlbum         = 4,
    Free128k             = 8,
    OnlyDownloadWithPaid = 16,
    OnlyOnlineWithPaid   = 32
};

struct Song {
    struct Ar {
        i64                      id;
        std::string              name;
        std::vector<std::string> alia;
    };
    struct Al {
        i64         id;
        std::string name;
        std::string picUrl;
    };

    struct Quality {
        i64 br;
        i64 size;
        i64 sr;
    };

    struct Privilege {
        i64         id;
        SongFee     fee;
        i64         payed;
        i64         st;
        i64         pl;    // 999000,
        i64         dl;    // 999000,
        i64         sp;    // 7,
        i64         cp;    // 1,
        i64         subp;  // 1,
        bool        cs;    // false,
        i64         maxbr; // 999000,
        i64         fl;    // 320000,
        bool        toast;
        i64         flag; // 128,
        bool        preSell;
        i64         playMaxbr;          // 999000,
        i64         downloadMaxbr;      // 999000,
        std::string maxBrLevel;         // "lossless",
        std::string playMaxBrLevel;     // "lossless",
        std::string downloadMaxBrLevel; // "lossless",
        std::string plLevel;            // "lossless",
        std::string dlLevel;            // "lossless",
        std::string flLevel;            // "exhigh",
                                        // rscl null,

        /*
        songMaxBr plLevel       dlLevel
        值 	      音质 	        比特率
        dobly     杜比 	        无
        hires     hires         1999
        lossless  无损 	        999
        exhigh    极高 	        320
        standard  标准 	        128
        none      不能播放/下载	0
        */

        /*
        songFee
        值 	说明            详细描述
        0 	免费            免费歌曲
        1 	会员            普通用户无法免费收听下载；会员可收听和下载所有音质
        4 	数字专辑        所有用户只能在商城购买数字专辑后，才能收听下载
        8 	128K            普通用户可免费收听128k音质，但不能下载；会员可收听和下载所有音质
        16 	只能付费下载
普通用户只能付费下载后使用，不提供在线收听；会员只能下载后使用，不能在线收听
        32  只能付费播放 普通用户只能付费后收听，不能下载；会员可以直接收听，但不能下载
        */
    };

    std::vector<Ar> ar;
    Al              al;
    i64             st;
    // std::optional<std::string> noCopyrightRcmd; // null, str, object
    i64                        rtype;
    i64                        pst;
    std::vector<std::string>   alia;
    i64                        pop;
    std::optional<std::string> rt;
    i64                        mst;
    i64                        cp;
    std::string                cf;
    Time                       dt;
    i64                        ftype;
    i64                        no;
    i64                        fee;
    i64                        mv;
    i64                        t;
    i64                        v;
    std::optional<Quality>     h;
    std::optional<Quality>     m;
    std::optional<Quality>     l;
    std::optional<Quality>     sq;
    std::optional<Quality>     hr;
    std::string                cd;
    std::string                name;
    i64                        id;

    std::optional<Privilege> privilege;

    // a	null
    // rtUrls	[]
    // songJumpInfo	null
    // rurl	null
    // crbt	null
    // rtUrl	null
    // djId	0
};

struct Artist {
    // topicPerson	0
    bool                       followed;
    std::vector<std::string>   alias;
    std::string                trans;
    i64                        musicSize;
    i64                        albumSize;
    std::optional<std::string> briefDesc;
    std::string                picUrl;
    std::string                img1v1Url;
    std::string                name;
    i64                        id;
    // std::string              picId_str;
    // std::string              img1v1Id_str;
};

struct Album {
    std::vector<Song>          songs;
    bool                       paid;
    bool                       onSale;
    i64                        mark;
    i64                        companyId;
    std::string                blurPicUrl;
    std::vector<std::string>   alias;
    std::vector<Artist>        artists;
    i64                        copyrightId;
    Artist                     artist;
    std::optional<std::string> briefDesc;
    Time                       publishTime;
    std::optional<std::string> company;
    std::string                picUrl;
    std::string                commentThreadId;
    std::optional<std::string> description;
    std::string                tags;
    i64                        status;
    std::string                subType;
    std::string                name;
    i64                        id;
    std::string                type;
    i64                        size;
    std::string                picId_str;
    // awardTags	null
    // info	{…}
};

struct Playlist {
    i64         id;
    std::string name;
    // coverImgId	109951167805071570
    std::string coverImgUrl;
    // coverImgId_str	"109951167805071571"
    // adType	0
    i64                userId;
    std::optional<i64> status;
    // opRecommend	false
    // highQuality	false
    // newImported	false

    i64 trackCount;
    i64 specialType;
    // privacy	0
    // trackUpdateTime	1678022963095
    std::optional<std::string> commentThreadId;
    i64                        playCount;
    // trackNumberUpdateTime	1678018138230
    // subscribedCount	4
    // cloudTrackCount	1
    // ordered	true
    std::optional<std::string>              description;
    std::optional<Time>                     createTime;
    std::optional<Time>                     updateTime;
    std::optional<std::vector<std::string>> tags;
    // updateFrequency	null
    // backgroundCoverId	0
    // backgroundCoverUrl	null
    // titleImage	0
    // titleImageUrl	null
    // englishTitle	null
    // officialPlaylistType	null
    // copied	false
    // relateResType	null
    // subscribers	[…]
    bool subscribed;
    // creator	{…}
    std::optional<std::vector<Song>> tracks;
    // videoIds	null
    // videos	null
    // trackIds	[…]
    // bannedTrackIds	null
    std::optional<i64> shareCount;
    std::optional<i64> commentCount;
    // remixVideo	null
    // sharedUsers	null
    // historySharedUsers	null
    // gradeStatus	"NONE"
    // score	null
    // algTags	null
};

JSON_DEFINE(Song);
JSON_DEFINE(Artist);
JSON_DEFINE(Album);
JSON_DEFINE(Playlist);
JSON_DEFINE(Time);

} // namespace model

} // namespace ncm
