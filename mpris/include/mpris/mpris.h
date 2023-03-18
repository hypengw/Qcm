#pragma once

#include <QObject>

namespace mpris
{

class MediaPlayer2;
class Mpris : public QObject {
    Q_OBJECT
public:
    Mpris(QObject* parent = nullptr);
    virtual ~Mpris();

    void registerService(QString serviceName);
    void unregisterService();

    MediaPlayer2* mediaplayer2() const;

private:
    MediaPlayer2* m_mp;
    QString       m_serviceName;
};
} // namespace mpris
