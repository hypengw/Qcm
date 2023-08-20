import QtQuick
import Qcm.App
import ".."

Item {
    required property QtObject playlist
    required property QtObject player

    function bindMpris() {
        const mpris = App.mpris;
        mpris.canPlay = true;
        mpris.canPause = true;
        mpris.canControl = true;
        mpris.playbackStatus = Qt.binding(() => {
                switch (player.playbackState) {
                case QcmPlayer.PlayingState:
                    return MprisMediaPlayer.Playing;
                case QcmPlayer.PausedState:
                    return MprisMediaPlayer.Paused;
                case QcmPlayer.StoppedState:
                    return MprisMediaPlayer.Stopped;
                }
            });
        mpris.loopStatus = Qt.binding(() => {
                switch (playlist.loopMode) {
                case Playlist.NoneLoop:
                    return MprisMediaPlayer.None;
                case Playlist.SingleLoop:
                    return MprisMediaPlayer.Track;
                case Playlist.ListLoop:
                case Playlist.ShuffleLoop:
                    return MprisMediaPlayer.Playlist;
                }
            });
        mpris.shuffle = Qt.binding(() => {
                return playlist.loopMode === Playlist.ShuffleLoop;
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
        const key = App.mpris.metakey;
        mpris.metadata = Qt.binding(() => {
                const meta = {};
                const song = playlist.cur;
                if (song.itemId.valid())
                    meta[key(MprisMediaPlayer.MetaTrackId)] = song.itemId.sid;
                if (root.song_cover)
                    meta[key(MprisMediaPlayer.MetaArtUrl)] = root.song_cover;
                meta[key(MprisMediaPlayer.MetaTitle)] = song.name;
                meta[key(MprisMediaPlayer.MetaAlbum)] = song.album.name;
                meta[key(MprisMediaPlayer.MetaAlbumArtist)] = song.artists.map(a => {
                        return a.name;
                    });
                meta[key(MprisMediaPlayer.MetaLength)] = player.duration * 1000;
                return meta;
            });
        // connect back
        mpris.playRequested.connect(player.play);
        mpris.pauseRequested.connect(player.pause);
        mpris.stopRequested.connect(player.stop);
        mpris.playPauseRequested.connect(() => {
                if (player.playbackState === QcmPlayer.PlayingState)
                    player.pause();
                else
                    player.play();
            });
        mpris.nextRequested.connect(playlist.next);
        mpris.previousRequested.connect(playlist.prev);
        mpris.loopStatusRequested.connect(s => {
                switch (s) {
                case MprisMediaPlayer.None:
                    playlist.loopMode = Playlist.NoneLoop;
                    break;
                case MprisMediaPlayer.Track:
                    playlist.loopMode = Playlist.SingleLoop;
                    break;
                case MprisMediaPlayer.Playlist:
                    playlist.loopMode = Playlist.ListLoop;
                    break;
                }
            });
        mpris.shuffleRequested.connect(shuffle => {
                if (shuffle)
                    playlist.loopMode = Playlist.ShuffleLoop;
                else
                    playlist.loopMode = Playlist.ListLoop;
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
        if (App.mpris)
            bindMpris();
    }
}
