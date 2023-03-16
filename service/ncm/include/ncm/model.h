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

JSON_DEFINE(Song);
JSON_DEFINE(Artist);
JSON_DEFINE(Album);
JSON_DEFINE(Time);

inline std::optional<Error> check_code(const json::njson&) {
    return std::nullopt;
    /*
    if (auto code = json::get<i64>(j, json::make_keys("code")); code.has_value()) {
        if (code == 200) return std::nullopt;
        return Error::push(fmt::format("not valied code: {}", code.value()), ERR_CTX);
    } else {
        return Error::push(code.error(), ERR_CTX);
    }
*/
}
} // namespace model

} // namespace ncm
