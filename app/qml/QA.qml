pragma Singleton
import QtQml
import QtQuick
import QtMultimedia
import Qt.labs.settings
import QcmApp

Item {
    property QtObject main_win: null
    readonly property string title: 'Qcm'
    readonly property alias user_info: m_querier_user.data
    readonly property bool is_login: m_querier_user.data.userId.valid()
    readonly property string loop_icon: switch (m_playlist.loopMode) {
    case Playlist.SingleLoop:
        return Theme.ic.repeat_one;
    case Playlist.ListLoop:
        return Theme.ic.loop;
    case Playlist.ShuffleLoop:
        return Theme.ic.shuffle;
    case Playlist.NoneLoop:
    default:
        return Theme.ic.trending_flat;
    }
    property alias querier_song: m_querier_song
    property alias querier_user: m_querier_user
    property alias player: m_player
    property alias playlist: m_playlist
    readonly property t_song cur_song: m_playlist.cur

    function join_name(objs, split) {
        const names = objs.map((o) => {
            return o.name;
        });
        return names.join(split);
    }

    function create_item(url, props, parent) {
        const com = Qt.createComponent(url);
        if (com.status === Component.Ready) {
            try {
                return com.createObject(parent, props);
            } catch (e) {
                console.error(e);
            }
        } else if (com.status == Component.Error) {
            console.error(com.errorString());
        }
    }

    function item_id_url(itemId) {
        switch (itemId.type) {
        case ItemId.Album:
            return 'qrc:/QcmApp/qml/page/AlbumDetailPage.qml';
        case ItemId.Playlist:
            return 'qrc:/QcmApp/qml/page/PlaylistDetailPage.qml';
        case ItemId.Artist:
            return 'qrc:/QcmApp/qml/page/ArtistDetailPage.qml';
        }
        return '';
    }

    function show_popup(url, props) {
        const popup = create_item(url, props, main_win);
        popup.closed.connect(() => {
            popup.destroy(1000);
        });
        popup.open();
    }

    function route(dest) {
        let url = dest;
        let props = {
        };
        switch (dest.type) {
        case ItemId.Album:
            url = 'qrc:/QcmApp/qml/page/AlbumDetailPage.qml';
            props = {
                "itemId": dest
            };
            break;
        case ItemId.Playlist:
            url = 'qrc:/QcmApp/qml/page/PlaylistDetailPage.qml';
            props = {
                "itemId": dest
            };
            break;
        case ItemId.Artist:
            url = 'qrc:/QcmApp/qml/page/ArtistDetailPage.qml';
            props = {
                "itemId": dest
            };
            break;
        }
        main_win.push_page(url, props);
    }

    Settings {
        property alias loop: m_playlist.loopMode

        category: 'play'
    }

    Playlist {
        id: m_playlist

        onCurChanged: function(refresh) {
            if (refresh)
                m_querier_song.ids = [];

            if (cur.itemId.valid())
                m_querier_song.ids = [cur.itemId.sid];

        }
    }

    UserAccountQuerier {
        id: m_querier_user

        onStatusChanged: {
            if (status === ApiQuerierBase.Finished)
                App.loginPost(data);

        }
    }

    SongUrlQuerier {
        id: m_querier_song

        autoReload: ids.length > 0
        onStatusChanged: {
            if (status === ApiQuerierBase.Error)
                m_player.stop();

        }
    }

    MediaPlayer {
        /*
            void loopStatusRequested(Loop_Status loopStatus);
            void rateRequested(double rate);
            void shuffleRequested(bool shuffle);
            void volumeRequested(double volume);
            void nextRequested();
            void openUriRequested(const QUrl& url);
            void pauseRequested();
            void playRequested();
            void playPauseRequested();
            void previousRequested();
            void seekRequested(qlonglong offset);
            void seeked(qlonglong position);
            void setPositionRequested(const QDBusObjectPath& trackId, qlonglong position);
            void stopRequested();
            */

        id: m_player

        readonly property bool playing: {
            switch (playbackState) {
            case MediaPlayer.PlayingState:
                return true;
            default:
                return false;
            }
        }

        function setPos(pos) {
            position = pos * duration;
        }

        function bindMpris() {
            App.mpris.canPlay = true;
            App.mpris.canPause = true;
            App.mpris.canControl = true;
            App.mpris.playbackStatus = Qt.binding(() => {
                switch (playbackState) {
                case MediaPlayer.PlayingState:
                    return MprisMediaPlayer.Playing;
                case MediaPlayer.PausedState:
                    return MprisMediaPlayer.Paused;
                case MediaPlayer.StoppedState:
                    return MprisMediaPlayer.Stopped;
                }
            });
            App.mpris.loopStatus = Qt.binding(() => {
                switch (m_playlist.loopMode) {
                case Playlist.NoneLoop:
                    return MprisMediaPlayer.None;
                case Playlist.SingleLoop:
                    return MprisMediaPlayer.Track;
                case Playlist.ListLoop:
                case Playlist.ShuffleLoop:
                    return MprisMediaPlayer.Playlist;
                }
            });
            App.mpris.shuffle = Qt.binding(() => {
                return m_playlist.loopMode === Playlist.ShuffleLoop;
            });
            App.mpris.volume = Qt.binding(() => {
                return audioOutput.volume;
            });
            App.mpris.position = Qt.binding(() => {
                return position * 1000;
            });
            App.mpris.canSeek = Qt.binding(() => {
                return seekable;
            });
            App.mpris.canGoNext = Qt.binding(() => {
                return m_playlist.canNext;
            });
            App.mpris.canGoPrevious = Qt.binding(() => {
                return m_playlist.canPrev;
            });
            const key = App.mpris.metakey;
            App.mpris.metadata = Qt.binding(() => {
                const meta = {
                };
                const song = m_playlist.cur;
                if (song.itemId.valid())
                    meta[key(MprisMediaPlayer.MetaTrackId)] = song.itemId.sid;

                meta[key(MprisMediaPlayer.MetaTitle)] = song.name;
                meta[key(MprisMediaPlayer.MetaAlbum)] = song.album.name;
                meta[key(MprisMediaPlayer.MetaLength)] = duration * 1000;
                return meta;
            });
            // connect back
            App.mpris.playRequested.connect(play);
            App.mpris.pauseRequested.connect(pause);
            App.mpris.stopRequested.connect(stop);
            App.mpris.playPauseRequested.connect(() => {
                if (playbackState === MediaPlayer.PlayingState)
                    pause();
                else
                    play();
            });
            App.mpris.nextRequested.connect(m_playlist.next);
            App.mpris.previousRequested.connect(m_playlist.prev);
            App.mpris.loopStatusRequested.connect((s) => {
                switch (s) {
                case MprisMediaPlayer.None:
                    m_playlist.loopMode = Playlist.NoneLoop;
                    break;
                case MprisMediaPlayer.Track:
                    m_playlist.loopMode = Playlist.SingleLoop;
                    break;
                case MprisMediaPlayer.Playlist:
                    m_playlist.loopMode = Playlist.ListLoop;
                    break;
                }
            });
            App.mpris.shuffleRequested.connect((shuffle) => {
                if (shuffle)
                    m_playlist.loopMode = Playlist.ShuffleLoop;
                else
                    m_playlist.loopMode = Playlist.ListLoop;
            });
            App.mpris.setPositionRequested.connect((_, pos) => {
                position = pos / 1000;
            });
        }

        source: {
            const songs = m_querier_song.data.songs;
            if (songs.length)
                return songs[0].url;

            return '';
        }
        onSourceChanged: (source) => {
            if (source)
                play();

        }
        onPlaybackStateChanged: {
            if (playbackState === MediaPlayer.StoppedState) {
                if (position / duration > 0.99)
                    m_playlist.next();

            }
        }
        Component.onCompleted: {
            if (App.mpris)
                bindMpris();

        }

        audioOutput: AudioOutput {
        }

    }

}
