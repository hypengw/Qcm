pragma Singleton
import QtQml
import QtQuick
import QtMultimedia
import Qt.labs.settings
import QcmApp
import "./part"

Item {
    id: root

    property QtObject main_win: null
    readonly property string title: 'Qcm'
    readonly property alias user_info: m_querier_user.data
    readonly property string user_setting_category: `user_${user_info.userId.sid}`
    readonly property alias user_song_set: m_querier_user_songlike.data
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
    property alias querier_user_song: m_querier_user_songlike
    property alias player: m_player
    property alias playlist: m_playlist
    readonly property t_song cur_song: m_playlist.cur
    property string song_cover: ''

    signal sig_like_album()
    signal sig_like_playlist()

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
        return popup;
    }

    function show_page_popup(url, props, popup_props = {
    }) {
        return show_popup('qrc:/QcmApp/qml/component/PagePopup.qml', Object.assign({
        }, {
            "source": url,
            "props": props
        }, popup_props));
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

    function toast(text, duration) {
        main_win.snake.show(text, duration);
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

    ApiContainer {
        UserAccountQuerier {
            id: m_querier_user

            readonly property bool loginOk: data.userId.valid()

            onLoginOkChanged: {
                if (loginOk)
                    App.loginPost(data);

            }
        }

        SongLikeQuerier {
            id: m_querier_user_songlike

            function like_song(song_id, is_like) {
                const qu = m_querier_radio_like;
                qu.trackId = song_id;
                qu.like = is_like;
                qu.query();
            }

            autoReload: m_querier_user.loginOk
        }

        RadioLikeQuerier {
            id: m_querier_radio_like

            onStatusChanged: {
                if (status === ApiQuerierBase.Finished) {
                    if (like)
                        m_querier_user_songlike.data.insert(trackId);
                    else
                        m_querier_user_songlike.data.remove(trackId);
                    m_querier_user_songlike.dataChanged();
                }
            }
            autoReload: false
        }

        SongUrlQuerier {
            id: m_querier_song

            autoReload: ids.length > 0
            onStatusChanged: {
                if (status === ApiQuerierBase.Error)
                    m_player.stop();

            }
        }

    }

    MediaPlayer {
        id: m_player

        readonly property bool playing: {
            switch (playbackState) {
            case MediaPlayer.PlayingState:
                return true;
            default:
                return false;
            }
        }

        signal seeked(real position)

        function seek(pos) {
            position = pos * duration;
            seeked(position * 1000);
        }

        function bindMpris() {
            const mpris = App.mpris;
            mpris.canPlay = true;
            mpris.canPause = true;
            mpris.canControl = true;
            mpris.playbackStatus = Qt.binding(() => {
                switch (playbackState) {
                case MediaPlayer.PlayingState:
                    return MprisMediaPlayer.Playing;
                case MediaPlayer.PausedState:
                    return MprisMediaPlayer.Paused;
                case MediaPlayer.StoppedState:
                    return MprisMediaPlayer.Stopped;
                }
            });
            mpris.loopStatus = Qt.binding(() => {
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
            mpris.shuffle = Qt.binding(() => {
                return m_playlist.loopMode === Playlist.ShuffleLoop;
            });
            mpris.volume = Qt.binding(() => {
                return audioOutput.volume;
            });
            mpris.position = Qt.binding(() => {
                return position * 1000;
            });
            mpris.canSeek = Qt.binding(() => {
                return seekable;
            });
            mpris.canGoNext = Qt.binding(() => {
                return m_playlist.canNext;
            });
            mpris.canGoPrevious = Qt.binding(() => {
                return m_playlist.canPrev;
            });
            const key = App.mpris.metakey;
            mpris.metadata = Qt.binding(() => {
                const meta = {
                };
                const song = m_playlist.cur;
                if (song.itemId.valid())
                    meta[key(MprisMediaPlayer.MetaTrackId)] = song.itemId.sid;

                if (root.song_cover)
                    meta[key(MprisMediaPlayer.MetaArtUrl)] = root.song_cover;

                meta[key(MprisMediaPlayer.MetaTitle)] = song.name;
                meta[key(MprisMediaPlayer.MetaAlbum)] = song.album.name;
                meta[key(MprisMediaPlayer.MetaAlbumArtist)] = song.artists.map((a) => {
                    return a.name;
                });
                meta[key(MprisMediaPlayer.MetaLength)] = duration * 1000;
                return meta;
            });
            // connect back
            mpris.playRequested.connect(play);
            mpris.pauseRequested.connect(pause);
            mpris.stopRequested.connect(stop);
            mpris.playPauseRequested.connect(() => {
                if (playbackState === MediaPlayer.PlayingState)
                    pause();
                else
                    play();
            });
            mpris.nextRequested.connect(m_playlist.next);
            mpris.previousRequested.connect(m_playlist.prev);
            mpris.loopStatusRequested.connect((s) => {
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
            mpris.shuffleRequested.connect((shuffle) => {
                if (shuffle)
                    m_playlist.loopMode = Playlist.ShuffleLoop;
                else
                    m_playlist.loopMode = Playlist.ListLoop;
            });
            mpris.setPositionRequested.connect((_, pos) => {
                position = pos / 1000;
                mpris.seeked(pos);
            });
            mpris.seekRequested.connect((offset) => {
                position = position + offset / 1000;
                mpris.seeked(position * 1000);
            });
            mpris.quitRequested.connect(() => {
                Qt.callLater(Qt.quit);
            });
            seeked.connect(mpris.seeked);
        }

        source: {
            const songs = m_querier_song.data.songs;
            if (songs.length) {
                const url = songs[0].url;
                return App.media_url(url, Qt.md5(songs[0].itemId.sid));
                return songs[0].url;
            }
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
