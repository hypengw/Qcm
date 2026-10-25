module;
#include <QUrl>
export module qcm.playback.io;
import qcm.core;
import wavsen.audio;

export namespace qcm
{
auto make_playback_io(const QUrl&) -> rc<wavsen::audio::MediaIo>;
}
