#pragma once

#include <string>
#include <set>

#include <QtQml/QQmlPropertyMap>

#include "Qcm/message/message.qpb.h"
#include "Qcm/model/share_store.hpp"
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

template<>
struct MsgTraits<msg::GetProviderMetasReq> {
    using Rsp                   = msg::GetProviderMetasRsp;
    static constexpr auto HasFn = &msg::QcmMessage::hasGetProviderMetasReq;
    static constexpr auto GetFn = &msg::QcmMessage::getProviderMetasReq;

    template<typename T>
    static auto set(msg::QcmMessage& m, T&& r) {
        m.setType(MessageTypeGadget::MessageType::GET_PROVIDER_METAS_REQ);
        m.setGetProviderMetasReq(std::forward<T>(r));
    }
};
template<>
struct MsgTraits<msg::GetProviderMetasRsp> {
    using Req                   = msg::GetProviderMetasReq;
    static constexpr auto HasFn = &msg::QcmMessage::hasGetProviderMetasRsp;
    static constexpr auto GetFn = &msg::QcmMessage::getProviderMetasRsp;
};

template<>
struct MsgTraits<msg::AddProviderReq> {
    using Rsp                   = msg::Rsp;
    static constexpr auto HasFn = &msg::QcmMessage::hasAddProviderReq;
    static constexpr auto GetFn = &msg::QcmMessage::addProviderReq;

    template<typename T>
    static auto set(msg::QcmMessage& m, T&& r) {
        m.setType(MessageTypeGadget::MessageType::ADD_PROVIDER_REQ);
        m.setAddProviderReq(std::forward<T>(r));
    }
};

template<>
struct MsgTraits<msg::GetAlbumsReq> {
    using Rsp                   = msg::GetAlbumsRsp;
    static constexpr auto HasFn = &msg::QcmMessage::hasGetAlbumsReq;
    static constexpr auto GetFn = &msg::QcmMessage::getAlbumsReq;

    template<typename T>
    static auto set(msg::QcmMessage& m, T&& r) {
        m.setType(MessageTypeGadget::MessageType::GET_ALBUMS_REQ);
        m.setGetAlbumsReq(std::forward<T>(r));
    }
};

template<>
struct MsgTraits<msg::GetAlbumReq> {
    using Rsp                   = msg::GetAlbumRsp;
    static constexpr auto HasFn = &msg::QcmMessage::hasGetAlbumReq;
    static constexpr auto GetFn = &msg::QcmMessage::getAlbumReq;

    template<typename T>
    static auto set(msg::QcmMessage& m, T&& r) {
        m.setType(MessageTypeGadget::MessageType::GET_ALBUM_REQ);
        m.setGetAlbumReq(std::forward<T>(r));
    }
};

template<>
struct MsgTraits<msg::GetArtistsReq> {
    using Rsp                   = msg::GetArtistsRsp;
    static constexpr auto HasFn = &msg::QcmMessage::hasGetArtistsReq;
    static constexpr auto GetFn = &msg::QcmMessage::getArtistsReq;

    template<typename T>
    static auto set(msg::QcmMessage& m, T&& r) {
        m.setType(MessageTypeGadget::MessageType::GET_ARTISTS_REQ);
        m.setGetArtistsReq(std::forward<T>(r));
    }
};

template<>
struct MsgTraits<msg::GetAlbumsRsp> {
    static constexpr auto HasFn = &msg::QcmMessage::hasGetAlbumsRsp;
    static constexpr auto GetFn = &msg::QcmMessage::getAlbumsRsp;
};

template<>
struct MsgTraits<msg::GetAlbumRsp> {
    static constexpr auto HasFn = &msg::QcmMessage::hasGetAlbumRsp;
    static constexpr auto GetFn = &msg::QcmMessage::getAlbumRsp;
};

template<>
struct MsgTraits<msg::GetArtistsRsp> {
    static constexpr auto HasFn = &msg::QcmMessage::hasGetArtistsRsp;
    static constexpr auto GetFn = &msg::QcmMessage::getArtistsRsp;
};

template<>
struct MsgTraits<msg::Rsp> {
    static constexpr auto HasFn = &msg::QcmMessage::hasRsp;
    static constexpr auto GetFn = &msg::QcmMessage::rsp;
};

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
class Album : public msg::model::Album {
    Q_GADGET
public:
};
class Song : public msg::model::Song {
    Q_GADGET
public:
};
} // namespace qcm::model

// -----------------------------------------

template<>
struct meta_model::ItemTrait<qcm::msg::model::Album> {
    using Self       = qcm::msg::model::Album;
    using key_type   = i64;
    using store_type = ShareStore<Self, std::pmr::polymorphic_allocator<Self>, qcm::ShareStoreExt>;
    static auto key(const Self& el) noexcept -> i64 { return el.id_proto().toLongLong(); }
};

template<>
struct meta_model::ItemTrait<qcm::msg::model::Artist> {
    using key_type = i64;
    static auto key(const qcm::msg::model::Artist& el) noexcept -> i64 {
        return el.id_proto().toLongLong();
    }
};

template<>
struct meta_model::ItemTrait<qcm::msg::model::Song> {
    using Self       = qcm::msg::model::Song;
    using key_type   = i64;
    using store_type = ShareStore<Self, std::pmr::polymorphic_allocator<Self>, qcm::ShareStoreExt>;
    static auto key(const Self& el) noexcept -> i64 { return el.id_proto().toLongLong(); }
};

template<>
struct meta_model::ItemTrait<qcm::msg::model::ProviderMeta> {
    using key_type = QString;
    static auto key(const qcm::msg::model::ProviderMeta& el) noexcept -> const key_type& {
        return el.typeName();
    }
};

template<>
struct meta_model::ItemTrait<qcm::msg::model::ProviderStatus> {
    using key_type = i64;
    static auto key(const qcm::msg::model::ProviderStatus& el) noexcept -> i64 {
        return el.id_proto().toLongLong();
    }
};

template<>
struct rstd::Impl<rstd::convert::From<google::protobuf::Value>, QVariant> {
    static auto from(google::protobuf::Value) -> QVariant;
};