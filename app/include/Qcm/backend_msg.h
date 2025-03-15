#pragma once

#include "Qcm/message/message.qpb.h"

import rstd.core;

namespace qcm::msg
{

struct Error {
    int code { 0 };
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
concept RspMsgCP = MsgCP<T> && requires { typename MsgTraits<T>::Req; };

template<typename T>
    requires ReqMsgCP<T> && MsgCP<typename MsgTraits<T>::Rsp>
auto get_rsp(msg::QcmMessage& msg) -> std::optional<typename MsgTraits<T>::Rsp> {
    using Rsp = typename MsgTraits<T>::Rsp;
    if (msg.*MsgTraits<Rsp>::HasFn()) {
        return msg.*MsgTraits<Rsp>::GetFn();
    }
    return {};
}

template<typename T>
    requires MsgCP<T>
auto get_msg(msg::QcmMessage& msg) -> std::optional<T> {
    if (msg.*MsgTraits<T>::HasFn()) {
        return msg.*MsgTraits<T>::GetFn();
    }
    return {};
}

template<>
struct MsgTraits<msg::GetProviderMetasReq> {
    using Rsp                   = msg::GetProviderMetasRsp;
    static constexpr auto HasFn = &msg::QcmMessage::hasGetProviderMetasReq;
    static constexpr auto GetFn = &msg::QcmMessage::getProviderMetasReq;
};
template<>
struct MsgTraits<msg::GetProviderMetasRsp> {
    using Req                   = msg::GetProviderMetasReq;
    static constexpr auto HasFn = &msg::QcmMessage::hasGetProviderMetasRsp;
    static constexpr auto GetFn = &msg::QcmMessage::getProviderMetasRsp;
};

} // namespace qcm::msg