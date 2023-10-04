pragma Singleton
import QtCore
import QtQml
import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

Item {
    id: root

    readonly property QA.t_song cur_song: m_playlist.cur
    readonly property bool is_login: m_querier_user.data.userId.valid()
    readonly property string loop_icon: switch (m_playlist.loopMode) {
    case QA.Playlist.SingleLoop:
        return MD.Token.icon.repeat_one;
    case QA.Playlist.ListLoop:
        return MD.Token.icon.loop;
    case QA.Playlist.ShuffleLoop:
        return MD.Token.icon.shuffle;
    case QA.Playlist.NoneLoop:
    default:
        return MD.Token.icon.trending_flat;
    }
    property QtObject main_win: null
    property alias player: m_player
    property alias playlist: m_playlist
    property alias querier_song: m_querier_song
    property alias querier_user: m_querier_user
    property alias querier_user_song: m_querier_user_songlike
    property string song_cover: ''
    property int color_scheme: MD.MdColorMgr.Light

    readonly property string title: 'Qcm'
    readonly property alias user_info: m_querier_user.data
    readonly property string user_setting_category: `user_${user_info.userId.sid}`
    readonly property alias user_song_set: m_querier_user_songlike.data

    signal sig_like_album
    signal sig_like_playlist
    signal sig_route(QA.RouteMsg msg)
    signal sig_route_special(string name)

    function create_item(url_or_comp, props, parent) {
        const com = (url_or_comp instanceof Component) ? url_or_comp : Qt.createComponent(url_or_comp);
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
        case QA.ItemIdType.Album:
            return 'qrc:/Qcm/App/qml/page/AlbumDetailPage.qml';
        case QA.ItemIdType.Playlist:
            return 'qrc:/Qcm/App/qml/page/PlaylistDetailPage.qml';
        case QA.ItemIdType.Artist:
            return 'qrc:/Qcm/App/qml/page/ArtistDetailPage.qml';
        }
        return '';
    }
    function join_name(objs, split) {
        const names = objs.map(o => {
                return o.name;
            });
        return names.join(split);
    }
    function route(dest) {
        let url = dest;
        let props = {};
        if (dest.objectType instanceof QA.ItemIdType) {
            url = item_id_url(dest);
            props = {
                "itemId": dest
            };
        }
        sig_route_special('main');
        const msg = m_comp_route_msg.createObject(root, {
                "qml": url,
                "props": props
            });
        sig_route(msg);
        msg.destroy(3000);
    }
    function show_page_popup(url, props, popup_props = {}) {
        return show_popup('qrc:/Qcm/App/qml/component/PagePopup.qml', Object.assign({}, {
                    "source": url,
                    "props": props
                }, popup_props));
    }
    function show_popup(url, props, parent) {
        const popup = create_item(url, props, parent ? parent : main_win);
        popup.closed.connect(() => {
                if (popup.destroy)
                    popup.destroy(1000);
            });
        popup.open();
        return popup;
    }
    function toast(text, duration) {
        main_win.snake.show(text, duration);
    }

    Component.onCompleted: {
        QA.App.errorOccurred.connect(s => {
                // ignore 'Operation aborted'
                if (!s.endsWith('Operation aborted.'))
                    root.toast(s, 5000);
            });
    }

    Component {
        id: m_comp_route_msg
        QA.RouteMsg {
        }
    }
    Settings {
        id: settings_play
        property alias loop: m_playlist.loopMode
        category: 'play'
    }
    Settings {
        id: settings_theme
        property alias color_scheme: root.color_scheme
        property color primary_color: MD.Token.color.accentColor
        category: 'theme'

        Component.onCompleted: {
            MD.Token.color.accentColor = primary_color;
            primary_color = Qt.binding(() => {
                return MD.Token.color.accentColor;
            });
            MD.Token.color.schemeTheme = Qt.binding(() => {
                return root.color_scheme;
            });
        }
    }
    QA.Playlist {
        id: m_playlist

        property var song_url_slot: null

        function iterLoopMode() {
            let mode = loopMode;
            switch (mode) {
            case QA.Playlist.NoneLoop:
                mode = QA.Playlist.SingleLoop;
                break;
            case QA.Playlist.SingleLoop:
                mode = QA.Playlist.ListLoop;
                break;
            case QA.Playlist.ListLoop:
                mode = QA.Playlist.ShuffleLoop;
                break;
            case QA.Playlist.ShuffleLoop:
                mode = QA.Playlist.NoneLoop;
                break;
            }
            loopMode = mode;
        }
        function songUrlSlot(key) {
            const status = m_querier_song.status;
            const songs = m_querier_song.data.songs;
            if (status === QA.ApiQuerierBase.Finished) {
                const song = songs.length ? songs[0] : null;
                const media_url = song ? QA.App.media_url(song.url, key) : '';
                m_player.source = media_url;
            } else if (status === QA.ApiQuerierBase.Error) {
                m_player.stop();
            }
        }

        onCurChanged: function (refresh) {
            const song_url_sig = m_querier_song.statusChanged;
            if (song_url_slot)
                song_url_sig.disconnect(song_url_slot);
            if (refresh)
                m_player.source = '';
            if (!cur.itemId.valid()) {
                m_player.stop();
                return;
            }
            const quality = parseInt(settings_play.value('play_quality', m_querier_song.level.toString()));
            const key = Qt.md5(`${cur.itemId.sid}, quality: ${quality}`);
            const file = QA.App.media_file(key);
            // seems empty url is true, use string
            if (file.toString()) {
                m_player.source = file;
                m_querier_song.ids = [];
            } else {
                song_url_slot = () => {
                    songUrlSlot(key);
                };
                song_url_sig.connect(song_url_slot);
                const songId = cur.itemId;
                if (refresh)
                    m_querier_song.ids = [];
                m_querier_song.level = quality;
                if (songId.valid())
                    m_querier_song.ids = [songId];
            }
        }
    }
    QA.UserAccountQuerier {
        id: m_querier_user

        readonly property bool loginOk: data.userId.valid()

        onLoginOkChanged: {
            if (loginOk)
                QA.App.loginPost(data);
        }
    }
    QA.SongLikeQuerier {
        id: m_querier_user_songlike
        function like_song(song_id, is_like) {
            const qu = m_querier_radio_like;
            qu.trackId = song_id;
            qu.like = is_like;
            qu.query();
        }

        autoReload: m_querier_user.loginOk
    }
    QA.RadioLikeQuerier {
        id: m_querier_radio_like
        autoReload: false

        onStatusChanged: {
            if (status === QA.ApiQuerierBase.Finished) {
                if (like)
                    m_querier_user_songlike.data.insert(trackId);
                else
                    m_querier_user_songlike.data.remove(trackId);
                m_querier_user_songlike.dataChanged();
            }
        }
    }
    QA.SongUrlQuerier {
        id: m_querier_song
        autoReload: ids.length > 0
    }

    QA.Mpris {
        id: m_mpris
        player: m_player
        playlist: m_playlist
    }

    QA.QcmPlayer {
        id: m_player

        readonly property bool seekable: true
        readonly property date duration_date: new Date(duration)
        readonly property bool playing: {
            switch (playbackState) {
            case QA.QcmPlayer.PlayingState:
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

        source: ''
        onSourceChanged: {
            if (source)
                play();
        }
        onPlaybackStateChanged: {
            if (playbackState === QA.QcmPlayer.StoppedState) {
                if (position / duration > 0.98)
                    m_playlist.next();
            }
        }
    }
}
