import QtQuick

import Qcm.App as QA

Item {
    required property QtObject playlist
    required property QA.QcmPlayer player

    function bindMpris() {
        const mpris = QA.App.mpris;
        if (!mpris)
            return;

        mpris.canPlay = true;
        mpris.canPause = true;
        mpris.canControl = true;
        mpris.playbackStatus = Qt.binding(() => {
            switch (player.playbackState) {
            case QA.Enum.PlayingState:
                return QA.MprisMediaPlayer.Playing;
            case QA.Enum.PausedState:
                return QA.MprisMediaPlayer.Paused;
            case QA.Enum.StoppedState:
            default:
                return QA.MprisMediaPlayer.Stopped;
            }
        });
        mpris.loopStatus = Qt.binding(() => {
            switch (playlist.loopMode) {
            case QA.Enum.NoneLoop:
                return QA.MprisMediaPlayer.None;
            case QA.Enum.SingleLoop:
                return QA.MprisMediaPlayer.Track;
            case QA.Enum.ListLoop:
            case QA.Enum.ShuffleLoop:
                return QA.MprisMediaPlayer.Playlist;
            }
        });
        mpris.shuffle = Qt.binding(() => {
            return playlist.loopMode === QA.Enum.ShuffleLoop;
        });
        mpris.volume = Qt.binding(() => {
            return 1.0;
            // return audioOutput.volume;
        });
        mpris.position = Qt.binding(() => {
            return player.position * 1000;
        });
        mpris.canSeek = Qt.binding(() => {
            return player.seekable;
        });
        mpris.canGoNext = Qt.binding(() => {
            return playlist.canNext;
        });
        mpris.canGoPrevious = Qt.binding(() => {
            return playlist.canPrev;
        });
        const key = QA.App.mpris.metakey;
        mpris.metadata = Qt.binding(() => {
            const meta = {};
            const song = playlist.currentSong;

            meta[key(QA.MprisMediaPlayer.MetaTrackId)] = QA.Util.mprisTrackid(song.itemId);
            if (root.song_cover)
                meta[key(QA.MprisMediaPlayer.MetaArtUrl)] = root.song_cover;
            meta[key(QA.MprisMediaPlayer.MetaTitle)] = song.name;
            meta[key(QA.MprisMediaPlayer.MetaAlbum)] = song.albumName;
            {
                const ex = QA.Store.extra(song.itemId);
                const artist = ex?.artists.map(a => {
                    return a.name;
                }) ?? [];
                meta[key(QA.MprisMediaPlayer.MetaArtist)] = artist;
            }
            {
                const ex = QA.Store.extra(song.albumId);
                const artist = ex?.artists.map(a => {
                    return a.name;
                }) ?? [];
                meta[key(QA.MprisMediaPlayer.MetaAlbumArtist)] = artist;
            }
            meta[key(QA.MprisMediaPlayer.MetaLength)] = player.duration * 1000;
            return meta;
        });
        // connect back
        mpris.playRequested.connect(player.play);
        mpris.pauseRequested.connect(player.pause);
        mpris.stopRequested.connect(player.stop);
        mpris.playPauseRequested.connect(() => {
            if (player.playbackState === QA.Enum.PlayingState)
                player.pause();
            else
                player.play();
        });
        mpris.nextRequested.connect(playlist.next);
        mpris.previousRequested.connect(playlist.prev);
        mpris.loopStatusRequested.connect(s => {
            switch (s) {
            case QA.MprisMediaPlayer.None:
                playlist.loopMode = QA.Enum.NoneLoop;
                break;
            case QA.MprisMediaPlayer.Track:
                playlist.loopMode = QA.Enum.SingleLoop;
                break;
            case QA.MprisMediaPlayer.Playlist:
                playlist.loopMode = QA.Enum.ListLoop;
                break;
            }
        });
        mpris.shuffleRequested.connect(shuffle => {
            if (shuffle)
                playlist.loopMode = QA.Enum.ShuffleLoop;
            else
                playlist.loopMode = QA.Enum.ListLoop;
        });
        mpris.setPositionRequested.connect((_, pos) => {
            player.position = pos / 1000;
            mpris.seeked(pos);
        });
        mpris.seekRequested.connect(offset => {
            player.position = player.position + offset / 1000;
            mpris.seeked(player.position * 1000);
        });
        mpris.quitRequested.connect(() => {
            Qt.callLater(Qt.quit);
        });
        player.seeked.connect(mpris.seeked);
    }

    Component.onCompleted: {
        if (QA.App.mpris)
            bindMpris();
    }
}
