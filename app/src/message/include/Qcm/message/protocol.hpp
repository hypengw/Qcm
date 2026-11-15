#pragma once
#include "message.qpb.h"
#include <QtCore/QString>

namespace qcm::protocol
{
inline constexpr quint32 Version       = 3;
inline constexpr quint32 EntityVersion = 2;

class Session {
public:
    void reset() {
        m_ready     = false;
        m_rejected  = false;
        m_ui_client = false;
        m_serial    = 0;
    }
    void begin(qint32 serial, bool ui_client = false) {
        reset();
        m_serial    = serial;
        m_ui_client = ui_client;
    }
    bool    ready() const { return m_ready; }
    bool    rejected() const { return m_rejected; }
    QString accept(const msg::QcmMessage& message) {
        if (message.protocolVersion() != Version)
            return QStringLiteral("Backend protocol mismatch: expected %1, received %2")
                .arg(Version)
                .arg(message.protocolVersion());
        if (m_rejected)
            return QStringLiteral("Backend message received after UI admission rejection");
        if (m_ready) {
            if (message.hasTestRsp())
                return QStringLiteral("Unexpected backend handshake response");
            return {};
        }
        if (! m_serial || message.id_proto() != m_serial || ! message.hasTestRsp() ||
            message.type() != msg::MessageTypeGadget::MessageType::TEST_RSP)
            return QStringLiteral("Backend message received before handshake");
        if (m_ui_client) {
            const auto response = message.testRsp();
            if (! response.hasUiAccepted())
                return QStringLiteral("Backend does not support UI admission");
            if (! response.uiAccepted()) {
                m_rejected = true;
                return {};
            }
        }
        if (! message.hasEntities() || message.entities().protocol() != EntityVersion ||
            message.entities().instance().isEmpty())
            return QStringLiteral("Backend entity protocol mismatch");
        m_ready = true;
        return {};
    }

private:
    qint32 m_serial {};
    bool   m_ready {};
    bool   m_rejected {};
    bool   m_ui_client {};
};
} // namespace qcm::protocol
