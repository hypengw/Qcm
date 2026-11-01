#pragma once
#include "message.qpb.h"
#include <QtCore/QString>

namespace qcm::protocol
{
inline constexpr quint32 Version = 2;

class Session {
public:
    void reset() {
        m_ready  = false;
        m_serial = 0;
    }
    void begin(qint32 serial) {
        reset();
        m_serial = serial;
    }
    bool    ready() const { return m_ready; }
    QString accept(const msg::QcmMessage& message) {
        if (message.protocolVersion() != Version)
            return QStringLiteral("Backend protocol mismatch: expected %1, received %2")
                .arg(Version)
                .arg(message.protocolVersion());
        if (m_ready) return {};
        if (! m_serial || message.id_proto() != m_serial || ! message.hasTestRsp() ||
            message.type() != msg::MessageTypeGadget::MessageType::TEST_RSP)
            return QStringLiteral("Backend message received before handshake");
        if (! message.hasEntities() || message.entities().protocol() != Version ||
            message.entities().instance().isEmpty())
            return QStringLiteral("Backend entity protocol mismatch");
        m_ready = true;
        return {};
    }

private:
    qint32 m_serial {};
    bool   m_ready {};
};
} // namespace qcm::protocol
