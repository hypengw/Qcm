#pragma once

#include "ncm/api.h"
#include "ncm/model.h"
#include "core/str_helper.h"

namespace ncm
{
namespace params
{
struct UserAccount {};
} // namespace params
} // namespace ncm

namespace ncm
{
namespace model
{
struct UserAccountProfile {
    UserId      userId;
    i64         userType;
    std::string nickname;
    // avatarImgId
    std::string avatarUrl;
    // backgroundImgId
    std::string backgroundUrl;
    // signature ""
    Time createTime;
    // userName "1_********xxx",
    i64 accountType;
    // shortUserName": "********xxx",
    // birthday: -xxxxxxx,
    i64 authority;
    i64 gender;
    i64 accountStatus;
    // province 520000,
    // city 522400,
    // authStatus": 0,
    std::optional<std::string> description;
    std::optional<std::string> detailDescription;
    bool                       defaultAvatar;
    // expertTags null,
    // experts null,
    // djStatus": 0,
    // locationStatus": 30,
    i64 vipType;
    // followed false,
    // mutual false,
    // authenticated false,
    // lastLoginTime 1678086690251,
    // lastLoginIP "220.166.3.155",
    // remarkName null,
    // viptypeVersion 1677200968603,
    // authenticationTypes 0,
    // avatarDetail null,
    // anchor false
};
} // namespace model

namespace api_model
{

struct UserAccount {
    static Result<UserAccount> parse(std::span<const byte> bs, const auto&) {
        return api_model::parse<UserAccount>(bs);
    }
    i64                                      code;
    std::optional<model::UserAccountProfile> profile;
};
JSON_DEFINE(UserAccount);

} // namespace api_model

namespace api
{

struct UserAccount {
    using in_type                      = params::UserAccount;
    using out_type                     = api_model::UserAccount;
    constexpr static Operation  oper   = Operation::PostOperation;
    constexpr static CryptoType crypto = CryptoType::WEAPI;

    std::string_view path() const { return "/weapi/nuser/account/get"; }
    UrlParams        query() const { return {}; }
    Params           body() const { return {}; }
    in_type          input;
};
static_assert(ApiCP<UserAccount>);

} // namespace api

} // namespace ncm
