use crate::{
    error::ProcessError,
    event::{BackendEvent, ServiceContext},
    msg,
};
use std::sync::atomic::{AtomicU8, Ordering};

pub const VERSION: u32 = 2;

pub struct Session {
    state: AtomicU8,
}
const WAITING: u8 = 0;
const READY: u8 = 1;
const CLOSED: u8 = 2;
impl Default for Session {
    fn default() -> Self {
        Self {
            state: AtomicU8::new(WAITING),
        }
    }
}

impl Session {
    pub fn ready(&self) -> bool {
        self.state.load(Ordering::Acquire) == READY
    }

    pub fn check(&self, message: &msg::QcmMessage) -> Result<(), ProcessError> {
        if message.protocol_version != VERSION {
            return Err(ProcessError::ProtocolVersion(message.protocol_version));
        }
        let hello = message.r#type() == msg::MessageType::TestReq
            && matches!(message.payload, Some(msg::qcm_message::Payload::TestReq(_)));
        if self.state.load(Ordering::Acquire) == CLOSED || (!hello && !self.ready()) {
            return Err(ProcessError::ProtocolNotReady);
        }
        Ok(())
    }

    pub fn close(&self) {
        self.state.store(CLOSED, Ordering::Release);
    }

    #[cfg(test)]
    pub fn testing() -> std::sync::Arc<Self> {
        std::sync::Arc::new(Self {
            state: AtomicU8::new(READY),
        })
    }
}

pub async fn complete_handshake(ctx: &ServiceContext) -> qcm_core::Result<()> {
    let previous =
        ctx.protocol
            .state
            .compare_exchange(WAITING, READY, Ordering::AcqRel, Ordering::Acquire);
    if previous == Err(CLOSED) {
        return Err(ProcessError::ProtocolNotReady.into());
    }
    if previous.is_ok() {
        if let Err(error) = ctx.backend_ev.send(BackendEvent::Frist).await {
            ctx.protocol.close();
            return Err(error.into());
        }
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn rejects_before_dispatch() {
        let session = Session::default();
        let mut message = msg::QcmMessage::default();
        for version in [0, 1, 3] {
            message.protocol_version = version;
            assert!(
                matches!(session.check(&message), Err(ProcessError::ProtocolVersion(v)) if v == version)
            );
        }
        message.protocol_version = VERSION;
        assert!(matches!(
            session.check(&message),
            Err(ProcessError::ProtocolNotReady)
        ));
        message.r#type = msg::MessageType::TestReq as i32;
        message.payload = Some(msg::qcm_message::Payload::TestReq(Default::default()));
        assert!(session.check(&message).is_ok());
        assert!(!session.ready());
        session.close();
        assert!(matches!(
            session.check(&message),
            Err(ProcessError::ProtocolNotReady)
        ));
    }
}
