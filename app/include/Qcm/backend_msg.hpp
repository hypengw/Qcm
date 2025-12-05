#pragma once

#include <string>
#include <set>

#include <QtQml/QQmlPropertyMap>

#include "Qcm/message/message.qpb.h"
#include "Qcm/model/share_store.hpp"
#include "Qcm/model/item_id.hpp"
#include "kstore/item_trait.hpp"

import qcm.core;

namespace qcm::msg
{

void merge_extra(QQmlPropertyMap&, const google::protobuf::Struct&,
                 const std::set<QStringView>& is_json_field);

struct Error {
    int         code { 0 };
    std::string message;
};

template<typename T>
struct MsgTraits;

template<typename T>
concept MsgCP = requires {
    MsgTraits<T>::HasFn;
    MsgTraits<T>::GetFn;
};
template<typename T>
concept ReqMsgCP = MsgCP<T> && requires { typename MsgTraits<T>::Rsp; };
template<typename T>
concept RspMsgCP = MsgCP<T>;

template<typename T>
    requires ReqMsgCP<T> && MsgCP<typename MsgTraits<T>::Rsp>
auto get_rsp(msg::QcmMessage& msg) -> Option<typename MsgTraits<T>::Rsp> {
    using Rsp = typename MsgTraits<T>::Rsp;
    if ((msg.*MsgTraits<Rsp>::HasFn)()) {
        return Some(typename MsgTraits<T>::Rsp((msg.*MsgTraits<Rsp>::GetFn)()));
    }
    return None();
}

template<typename T>
    requires MsgCP<T>
auto get_msg(msg::QcmMessage& msg) -> Option<T> {
    if (msg.*MsgTraits<T>::HasFn()) {
        return Some(msg.*MsgTraits<T>::GetFn());
    }
    return None();
}

template<typename T>
struct FilterTraits {};

#define QCM_FILTER_TRAITS(TYPE, FTYPE)                                              \
    template<>                                                                      \
    struct FilterTraits<TYPE> {                                                     \
        constexpr static auto type { filter::FilterTypeGadget::FilterType::FTYPE }; \
    };

// Add these macros before template specializations
#define QCM_MSG_TRAITS_COMMON(TYPE, GET)                       \
    static constexpr auto HasFn = &msg::QcmMessage::has##TYPE; \
    static constexpr auto GetFn = &msg::QcmMessage::GET;

#define QCM_MSG_TRAITS_REQ(TYPE, RSP_TYPE, MSG_TYPE, GET)        \
    template<>                                                   \
    struct MsgTraits<msg::TYPE> {                                \
        using Rsp = msg::RSP_TYPE;                               \
        QCM_MSG_TRAITS_COMMON(TYPE, GET)                         \
        template<typename T>                                     \
        static auto set(msg::QcmMessage& m, T&& r) {             \
            m.setType(MessageTypeGadget::MessageType::MSG_TYPE); \
            m.set##TYPE(std::forward<T>(r));                     \
        }                                                        \
    };

#define QCM_MSG_TRAITS_RSP(TYPE, REQ_TYPE, GET) \
    template<>                                  \
    struct MsgTraits<msg::TYPE> {               \
        using Req = msg::REQ_TYPE;              \
        QCM_MSG_TRAITS_COMMON(TYPE, GET)        \
    };

// Replace existing specializations with macro usage
QCM_MSG_TRAITS_REQ(GetProviderMetasReq, GetProviderMetasRsp, GET_PROVIDER_METAS_REQ,
                   getProviderMetasReq)
QCM_MSG_TRAITS_RSP(GetProviderMetasRsp, GetProviderMetasReq, getProviderMetasRsp)
QCM_MSG_TRAITS_REQ(AddProviderReq, Rsp, ADD_PROVIDER_REQ, addProviderReq)
QCM_MSG_TRAITS_REQ(DeleteProviderReq, Rsp, DELETE_PROVIDER_REQ, deleteProviderReq)
QCM_MSG_TRAITS_REQ(ReplaceProviderReq, Rsp, REPLACE_PROVIDER_REQ, replaceProviderReq)
QCM_MSG_TRAITS_REQ(UpdateProviderReq, UpdateProviderRsp, UPDATE_PROVIDER_REQ, updateProviderReq)
QCM_MSG_TRAITS_RSP(UpdateProviderRsp, UpdateProviderReq, updateProviderRsp)
QCM_MSG_TRAITS_REQ(AuthProviderReq, AuthProviderRsp, AUTH_PROVIDER_REQ, authProviderReq)
QCM_MSG_TRAITS_RSP(AuthProviderRsp, AuthProviderReq, authProviderRsp)
QCM_MSG_TRAITS_REQ(CreateTmpProviderReq, CreateTmpProviderRsp, CREATE_TMP_PROVIDER_REQ,
                   createTmpProviderReq)
QCM_MSG_TRAITS_RSP(CreateTmpProviderRsp, CreateTmpProviderReq, createTmpProviderRsp)
QCM_MSG_TRAITS_REQ(DeleteTmpProviderReq, Rsp, DELETE_TMP_PROVIDER_REQ, deleteTmpProviderReq)

QCM_MSG_TRAITS_REQ(QrAuthUrlReq, QrAuthUrlRsp, QR_AUTH_URL_REQ, qrAuthUrlReq)
QCM_MSG_TRAITS_RSP(QrAuthUrlRsp, QrAuthUrlReq, qrAuthUrlRsp)

QCM_MSG_TRAITS_REQ(GetAlbumsReq, GetAlbumsRsp, GET_ALBUMS_REQ, getAlbumsReq)
QCM_MSG_TRAITS_REQ(GetAlbumReq, GetAlbumRsp, GET_ALBUM_REQ, getAlbumReq)
QCM_MSG_TRAITS_RSP(GetAlbumsRsp, GetAlbumsReq, getAlbumsRsp)
QCM_MSG_TRAITS_RSP(GetAlbumRsp, GetAlbumReq, getAlbumRsp)

QCM_MSG_TRAITS_REQ(GetArtistsReq, GetArtistsRsp, GET_ARTISTS_REQ, getArtistsReq)
QCM_MSG_TRAITS_RSP(GetArtistsRsp, GetArtistsReq, getArtistsRsp)
QCM_MSG_TRAITS_REQ(GetAlbumArtistsReq, GetAlbumArtistsRsp, GET_ALBUM_ARTISTS_REQ,
                   getAlbumArtistsReq)
QCM_MSG_TRAITS_RSP(GetAlbumArtistsRsp, GetAlbumArtistsReq, getAlbumArtistsRsp)
QCM_MSG_TRAITS_REQ(GetArtistReq, GetArtistRsp, GET_ARTIST_REQ, getArtistReq)
QCM_MSG_TRAITS_RSP(GetArtistRsp, GetArtistReq, getArtistRsp)
QCM_MSG_TRAITS_REQ(GetArtistAlbumReq, GetArtistAlbumRsp, GET_ARTIST_ALBUM_REQ, getArtistAlbumReq)
QCM_MSG_TRAITS_RSP(GetArtistAlbumRsp, GetArtistAlbumReq, getArtistAlbumRsp)

QCM_MSG_TRAITS_REQ(GetMixsReq, GetMixsRsp, GET_MIXS_REQ, getMixsReq)
QCM_MSG_TRAITS_RSP(GetMixsRsp, GetMixsReq, getMixsRsp)
QCM_MSG_TRAITS_REQ(GetMixReq, GetMixRsp, GET_MIX_REQ, getMixReq)
QCM_MSG_TRAITS_RSP(GetMixRsp, GetMixReq, getMixRsp)
QCM_MSG_TRAITS_REQ(CreateMixReq, CreateMixRsp, CREATE_MIX_REQ, createMixReq)
QCM_MSG_TRAITS_RSP(CreateMixRsp, CreateMixReq, createMixRsp)
QCM_MSG_TRAITS_REQ(GetMixSongsReq, GetMixSongsRsp, GET_MIX_SONGS_REQ, getMixSongsReq)
QCM_MSG_TRAITS_RSP(GetMixSongsRsp, GetMixSongsReq, getMixSongsRsp)
QCM_MSG_TRAITS_REQ(DeleteMixReq, Rsp, DELETE_MIX_REQ, deleteMixReq)
QCM_MSG_TRAITS_REQ(MixManipulateReq, MixManipulateRsp, MIX_MANIPULATE_REQ, mixManipulateReq)
QCM_MSG_TRAITS_RSP(MixManipulateRsp, MixManipulateReq,  mixManipulateRsp)

QCM_MSG_TRAITS_REQ(GetRemoteMixsReq, GetRemoteMixsRsp, GET_REMOTE_MIXS_REQ, getRemoteMixsReq)
QCM_MSG_TRAITS_RSP(GetRemoteMixsRsp, GetRemoteMixsReq, getRemoteMixsRsp)

QCM_MSG_TRAITS_REQ(SearchReq, SearchRsp, SEARCH_REQ, searchReq)
QCM_MSG_TRAITS_RSP(SearchRsp, SearchReq, searchRsp)

QCM_MSG_TRAITS_REQ(SyncReq, SyncRsp, SYNC_REQ, syncReq)
QCM_MSG_TRAITS_RSP(SyncRsp, SyncReq, syncRsp)
QCM_MSG_TRAITS_REQ(GetSubtitleReq, GetSubtitleRsp, GET_SUBTITLE_REQ, getSubtitleReq)
QCM_MSG_TRAITS_RSP(GetSubtitleRsp, GetSubtitleReq, getSubtitleRsp)
QCM_MSG_TRAITS_REQ(SetFavoriteReq, Rsp, SET_FAVORITE_REQ, setFavoriteReq)

QCM_MSG_TRAITS_REQ(GetStorageInfoReq, GetStorageInfoRsp, GET_STORAGE_INFO_REQ, getStorageInfoReq)
QCM_MSG_TRAITS_RSP(GetStorageInfoRsp, GetStorageInfoReq, getStorageInfoRsp)

QCM_MSG_TRAITS_REQ(PlaylogReq, Rsp, PLAYLOG_REQ, playlogReq)

template<>
struct MsgTraits<msg::Rsp> {
    QCM_MSG_TRAITS_COMMON(Rsp, rsp)
};

QCM_FILTER_TRAITS(filter::TitleFilter, FILTER_TYPE_TITLE)
QCM_FILTER_TRAITS(filter::NameFilter, FILTER_TYPE_NAME)
QCM_FILTER_TRAITS(filter::TrackCountFilter, FILTER_TYPE_TRACK_COUNT)
QCM_FILTER_TRAITS(filter::AlbumArtistIdFilter, FILTER_TYPE_ALBUM_ARTIST_ID)
QCM_FILTER_TRAITS(filter::AlbumTitleFilter, FILTER_TYPE_ALBUM_TITLE)
QCM_FILTER_TRAITS(filter::ArtistIdFilter, FILTER_TYPE_ARTIST_ID)
QCM_FILTER_TRAITS(filter::ArtistNameFilter, FILTER_TYPE_ARTIST_NAME)
QCM_FILTER_TRAITS(filter::YearFilter, FILTER_TYPE_YEAR)
QCM_FILTER_TRAITS(filter::DurationFilter, FILTER_TYPE_DURATION)
QCM_FILTER_TRAITS(filter::AddedDateFilter, FILTER_TYPE_ADDED_DATE)
QCM_FILTER_TRAITS(filter::TypeFilter, FILTER_TYPE_TYPE)
QCM_FILTER_TRAITS(filter::DiscCountFilter, FILTER_TYPE_DISC_COUNT)

#undef QCM_MSG_TRAITS_COMMON
#undef QCM_MSG_TRAITS_REQ
#undef QCM_MSG_TRAITS_RSP
#undef QCM_FILTER_TRAITS

} // namespace qcm::msg

template<>
struct std::formatter<qcm::msg::Error> : std::formatter<std::string_view> {
    template<typename Ctx>
    auto format(qcm::msg::Error err, Ctx& ctx) const -> typename Ctx::iterator {
        return std::formatter<std::string_view>::format(
            std::format("{}({})", err.message, err.code), ctx);
    }
};

template<>
struct std::formatter<qcm::msg::MessageTypeGadget::MessageType> : std::formatter<std::string_view> {
    using MessageType = qcm::msg::MessageTypeGadget::MessageType;
    template<typename Ctx>
    auto format(MessageType type, Ctx& ctx) const -> typename Ctx::iterator {
        return std::formatter<std::string_view>::format(
            std::string_view { QMetaEnum::fromType<MessageType>().valueToKey((int)type) }, ctx);
    }
};

// -----------------------------------------

namespace qcm::model
{
class Song;

template<typename T>
void model_init(T* self) {
    self->setId_proto(-1);
    if constexpr (std::same_as<T, Song>) {
        self->setAlbumId(-1);
    }
}

auto common_extra(model::ItemId id) -> QQmlPropertyMap*;

#define QCM_MODEL_COMMON(T, _ItemType)                                                 \
    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId FINAL)            \
    Q_PROPERTY(QQmlPropertyMap* extra READ extra FINAL)                                \
                                                                                       \
public:                                                                                \
    T(): msg::model::T() { model_init<T>(this); }                                      \
    T(const model::T& o): msg::model::T(o) {}                                          \
    T(model::T&& o) noexcept: msg::model::T(std::move(o)) {}                           \
    T& operator=(const model::T& o) {                                                  \
        msg::model::T::operator=(o);                                                   \
        return *this;                                                                  \
    }                                                                                  \
    T& operator=(model::T&& o) noexcept {                                              \
        msg::model::T::operator=(std::move(o));                                        \
        return *this;                                                                  \
    }                                                                                  \
    T(const msg::model::T& o): msg::model::T(o) {}                                     \
    T(msg::model::T&& o) noexcept: msg::model::T(std::move(o)) {}                      \
    auto extra() const noexcept -> QQmlPropertyMap* { return common_extra(itemId()); } \
    auto itemId() const -> qcm::model::ItemId {                                        \
        return { enums::ItemType::_ItemType, this->id_proto() };                       \
    }                                                                                  \
    void setItemId(const qcm::model::ItemId& v) { this->setId_proto(v.id()); }

class Album : public msg::model::Album {
    Q_GADGET
    QML_VALUE_TYPE(album)

    QCM_MODEL_COMMON(Album, ItemAlbum)
};

class Song : public msg::model::Song {
    Q_GADGET
    QML_VALUE_TYPE(song)
    Q_PROPERTY(qcm::model::ItemId albumId READ albumItemId WRITE setAlbumItemId FINAL)
    Q_PROPERTY(QString albumName READ albumName FINAL)
    QCM_MODEL_COMMON(Song, ItemSong)

public:
    auto albumItemId() const -> ItemId {
        return ItemId { enums::ItemType::ItemAlbum, msg::model::Song::albumId() };
    }
    void setAlbumItemId(ItemId id) { msg::model::Song::setAlbumId(id.id()); }
    auto albumName() const -> QString;
};

class Artist : public msg::model::Artist {
    Q_GADGET
    QML_VALUE_TYPE(artist)

    // prefer album artist here
    // and manually set to song artist if needed
    QCM_MODEL_COMMON(Artist, ItemAlbumArtist)
};

class Mix : public msg::model::Mix {
    Q_GADGET
    QML_VALUE_TYPE(mix)

    QCM_MODEL_COMMON(Mix, ItemMix)
private:
    auto libraryId() const -> i64 { return -1; }
};

class ProviderStatus : public msg::model::ProviderStatus {
    Q_GADGET
    QCM_MODEL_COMMON(ProviderStatus, ItemProvider)
private:
    auto libraryId() const -> i64 { return -1; }
};

#undef QCM_MODEL_COMMON

} // namespace qcm::model

// -----------------------------------------

template<>
struct kstore::ItemTrait<qcm::model::Album> {
    using Self       = qcm::model::Album;
    using key_type   = i64;
    using store_type = ShareStore<Self, std::pmr::polymorphic_allocator<Self>, qcm::ShareStoreExt>;
    static auto key(const Self& el) noexcept -> i64 { return el.id_proto(); }
};

template<>
struct kstore::ItemTrait<qcm::model::Artist> {
    using Self       = qcm::model::Artist;
    using key_type   = i64;
    using store_type = ShareStore<Self, std::pmr::polymorphic_allocator<Self>, qcm::ShareStoreExt>;
    static auto key(const Self& el) noexcept -> i64 { return el.id_proto(); }
};

template<>
struct kstore::ItemTrait<qcm::model::Song> {
    using Self       = qcm::model::Song;
    using key_type   = i64;
    using store_type = ShareStore<Self, std::pmr::polymorphic_allocator<Self>, qcm::ShareStoreExt>;
    static auto key(const Self& el) noexcept -> i64 { return el.id_proto(); }
};

template<>
struct kstore::ItemTrait<qcm::model::Mix> {
    using Self       = qcm::model::Mix;
    using key_type   = i64;
    using store_type = ShareStore<Self, std::pmr::polymorphic_allocator<Self>, qcm::ShareStoreExt>;
    static auto key(const Self& el) noexcept -> i64 { return el.id_proto(); }
};

template<>
struct kstore::ItemTrait<qcm::msg::model::ProviderMeta> {
    using key_type = QString;
    static auto key(const qcm::msg::model::ProviderMeta& el) noexcept -> const key_type& {
        return el.typeName();
    }
};

template<>
struct kstore::ItemTrait<qcm::model::ProviderStatus> {
    using Self     = qcm::model::ProviderStatus;
    using key_type = i64;
    static auto key(const Self& el) noexcept -> i64 { return el.id_proto(); }
};

template<>
struct rstd::Impl<rstd::convert::From<google::protobuf::Value>, QVariant> {
    static auto from(google::protobuf::Value) -> QVariant;
};

template<>
struct rstd::Impl<rstd::convert::From<qcm::enums::ItemType>,
                  qcm::msg::model::ItemTypeGadget::ItemType> {
    using in_t  = qcm::enums::ItemType;
    using out_t = qcm::msg::model::ItemTypeGadget::ItemType;
    static auto from(qcm::enums::ItemType) -> out_t;
};