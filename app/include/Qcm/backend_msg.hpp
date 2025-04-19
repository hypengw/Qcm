#pragma once

#include <string>
#include <set>

#include <QtQml/QQmlPropertyMap>

#include "Qcm/message/message.qpb.h"
#include "Qcm/model/share_store.hpp"
#include "Qcm/model/item_id.hpp"
#include "meta_model/item_trait.hpp"

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
QCM_MSG_TRAITS_REQ(AddProviderReq, AddProviderRsp, ADD_PROVIDER_REQ, addProviderReq)
QCM_MSG_TRAITS_RSP(AddProviderRsp, AddProviderReq, addProviderRsp)

QCM_MSG_TRAITS_REQ(QrAuthUrlReq, QrAuthUrlRsp, QR_AUTH_URL_REQ, qrAuthUrlReq)
QCM_MSG_TRAITS_RSP(QrAuthUrlRsp, QrAuthUrlReq, qrAuthUrlRsp)

QCM_MSG_TRAITS_REQ(GetAlbumsReq, GetAlbumsRsp, GET_ALBUMS_REQ, getAlbumsReq)
QCM_MSG_TRAITS_REQ(GetAlbumReq, GetAlbumRsp, GET_ALBUM_REQ, getAlbumReq)
QCM_MSG_TRAITS_RSP(GetAlbumsRsp, GetAlbumsReq, getAlbumsRsp)
QCM_MSG_TRAITS_RSP(GetAlbumRsp, GetAlbumReq, getAlbumRsp)

QCM_MSG_TRAITS_REQ(GetArtistsReq, GetArtistsRsp, GET_ARTISTS_REQ, getArtistsReq)
QCM_MSG_TRAITS_RSP(GetArtistsRsp, GetArtistsReq, getArtistsRsp)
QCM_MSG_TRAITS_REQ(GetArtistReq, GetArtistRsp, GET_ARTIST_REQ, getArtistReq)
QCM_MSG_TRAITS_RSP(GetArtistRsp, GetArtistReq, getArtistRsp)
QCM_MSG_TRAITS_REQ(GetArtistAlbumReq, GetArtistAlbumRsp, GET_ARTIST_ALBUM_REQ, getArtistAlbumReq)
QCM_MSG_TRAITS_RSP(GetArtistAlbumRsp, GetArtistAlbumReq, getArtistAlbumRsp)

QCM_MSG_TRAITS_REQ(GetMixsReq, GetMixsRsp, GET_MIXS_REQ, getMixsReq)
QCM_MSG_TRAITS_RSP(GetMixsRsp, GetMixsReq, getMixsRsp)
QCM_MSG_TRAITS_REQ(GetMixReq, GetMixRsp, GET_MIX_REQ, getMixReq)
QCM_MSG_TRAITS_RSP(GetMixRsp, GetMixReq, getMixRsp)

QCM_MSG_TRAITS_REQ(SyncReq, SyncRsp, SYNC_REQ, syncReq)
QCM_MSG_TRAITS_RSP(SyncRsp, SyncReq, syncRsp)

template<>
struct MsgTraits<msg::Rsp> {
    QCM_MSG_TRAITS_COMMON(Rsp, rsp)
};

#undef QCM_MSG_TRAITS_COMMON
#undef QCM_MSG_TRAITS_REQ
#undef QCM_MSG_TRAITS_RSP

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
    if constexpr(std::same_as<T, Song>) {
        self->setAlbumId(-1);
    }
}

#define QCM_MODEL_COMMON(T, _ItemType)                                              \
    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId FINAL)         \
                                                                                    \
public:                                                                             \
    T(): msg::model::T() { model_init<T>(this); }                                   \
    T(const model::T& o): msg::model::T(o) {}                                       \
    T(model::T&& o) noexcept: msg::model::T(std::move(o)) {}                        \
    T& operator=(const model::T& o) {                                               \
        msg::model::T::operator=(o);                                                \
        return *this;                                                               \
    }                                                                               \
    T& operator=(model::T&& o) noexcept {                                           \
        msg::model::T::operator=(std::move(o));                                     \
        return *this;                                                               \
    }                                                                               \
    T(const msg::model::T& o): msg::model::T(o) {}                                  \
    T(msg::model::T&& o) noexcept: msg::model::T(std::move(o)) {}                   \
    auto itemId() const -> qcm::model::ItemId {                                     \
        return { enums::ItemType::_ItemType, this->id_proto(), this->libraryId() }; \
    }                                                                               \
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
        return ItemId { enums::ItemType::ItemAlbum,
                        msg::model::Song::albumId(),
                        this->libraryId() };
    }
    void setAlbumItemId(ItemId id) { msg::model::Song::setAlbumId(id.id()); }
    auto albumName() const -> QString;
};

class Artist : public msg::model::Artist {
    Q_GADGET
    QML_VALUE_TYPE(artist)

    QCM_MODEL_COMMON(Artist, ItemArtist)
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
struct meta_model::ItemTrait<qcm::model::Album> {
    using Self       = qcm::model::Album;
    using key_type   = i64;
    using store_type = ShareStore<Self, std::pmr::polymorphic_allocator<Self>, qcm::ShareStoreExt>;
    static auto key(const Self& el) noexcept -> i64 { return el.id_proto(); }
};

template<>
struct meta_model::ItemTrait<qcm::model::Artist> {
    using Self       = qcm::model::Artist;
    using key_type   = i64;
    using store_type = ShareStore<Self, std::pmr::polymorphic_allocator<Self>, qcm::ShareStoreExt>;
    static auto key(const Self& el) noexcept -> i64 { return el.id_proto(); }
};

template<>
struct meta_model::ItemTrait<qcm::model::Song> {
    using Self       = qcm::model::Song;
    using key_type   = i64;
    using store_type = ShareStore<Self, std::pmr::polymorphic_allocator<Self>, qcm::ShareStoreExt>;
    static auto key(const Self& el) noexcept -> i64 { return el.id_proto(); }
};

template<>
struct meta_model::ItemTrait<qcm::model::Mix> {
    using Self       = qcm::model::Mix;
    using key_type   = i64;
    using store_type = ShareStore<Self, std::pmr::polymorphic_allocator<Self>, qcm::ShareStoreExt>;
    static auto key(const Self& el) noexcept -> i64 { return el.id_proto(); }
};

template<>
struct meta_model::ItemTrait<qcm::msg::model::ProviderMeta> {
    using key_type = QString;
    static auto key(const qcm::msg::model::ProviderMeta& el) noexcept -> const key_type& {
        return el.typeName();
    }
};

template<>
struct meta_model::ItemTrait<qcm::model::ProviderStatus> {
    using Self     = qcm::model::ProviderStatus;
    using key_type = i64;
    static auto key(const Self& el) noexcept -> i64 { return el.id_proto(); }
};

template<>
struct rstd::Impl<rstd::convert::From<google::protobuf::Value>, QVariant> {
    static auto from(google::protobuf::Value) -> QVariant;
};