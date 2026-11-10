module;
#include <rstd/macro.hpp>
export module qcm.backend:protocol.session;
export import :event;
using rstd::sync::atomic::Atomic;
using rstd::sync::atomic::Ordering;
using namespace rstd::prelude;

export namespace qcm::backend
{
enum class ProtocolState : unsigned char
{
    Waiting,
    Ready,
    Closed
};

class ProtocolSession {
    Atomic<ProtocolState>     state_ { ProtocolState::Waiting };
    rstd::async::Mutex<empty> handshake_ { empty {} };

public:
    auto        state() const -> ProtocolState { return state_.load(Ordering::Acquire); }
    auto        ready() const -> bool { return state() == ProtocolState::Ready; }
    static auto is_handshake(const QcmMessage& message) -> bool {
        return message.f_type == i32(static_cast<int>(p_qcm::p_msg::E_MessageType::v_TEST_REQ)) &&
               message.o_payload.is_some() && message.o_payload->is_test_req();
    }
    static auto is_handshake_response(const QcmMessage& message) -> bool {
        return message.f_type == i32(static_cast<int>(p_qcm::p_msg::E_MessageType::v_TEST_RSP)) &&
               message.o_payload.is_some() && message.o_payload->is_test_rsp();
    }
    auto check(const QcmMessage& message) const -> ProcessResult<empty> {
        if (message.f_protocol_version != qcm_protocol_version)
            return Err(ProcessError::ProtocolVersion(message.f_protocol_version));
        auto current = state();
        if (current == ProtocolState::Closed ||
            (current != ProtocolState::Ready && ! is_handshake(message)))
            return Err(ProcessError::ProtocolNotReady());
        return Ok(empty {});
    }
    auto handshake() const -> rstd::async::MutexLock<empty> { return handshake_.lock(); }
    // Call only after the handshake response has been accepted by the transport.
    auto complete_handshake() -> ProcessResult<bool> {
        auto previous = ProtocolState::Waiting;
        if (state_.compare_exchange_strong(
                previous, ProtocolState::Ready, Ordering::AcqRel, Ordering::Acquire))
            return Ok(true);
        if (previous == ProtocolState::Closed) return Err(ProcessError::ProtocolNotReady());
        return Ok(false);
    }
    void close() { state_.store(ProtocolState::Closed, Ordering::Release); }
};
} // namespace qcm::backend
