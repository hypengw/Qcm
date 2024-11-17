#include "mpris/mpris.h"
#include "mpris/mediaplayer2.h"

#include <QDBusConnection>

using namespace mpris;
using namespace Qt::Literals::StringLiterals;

namespace
{
static const QString ServiceNamePrefix { u"org.mpris.MediaPlayer2."_s };
}

Mpris::Mpris(QObject* parent): QObject(parent), m_mp(new MediaPlayer2(this)) {};

Mpris::~Mpris() { unregisterService(); }

mpris::MediaPlayer2* Mpris::mediaplayer2() const { return m_mp; }

void Mpris::registerService(QString serviceName) {
    QString mspris2Name { ServiceNamePrefix + serviceName };

    if (! QDBusConnection::sessionBus().registerService(mspris2Name)) {
        return;
    }
    m_serviceName = serviceName;
}

void Mpris::unregisterService() {
    if (! m_serviceName.isEmpty()) {
        QDBusConnection::sessionBus().unregisterService(ServiceNamePrefix + m_serviceName);
    }
}
