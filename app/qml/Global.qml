pragma Singleton
import QtCore
import QtQml
import QtQuick
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

QA.GlobalWrapper {
    id: root

    readonly property QA.t_song cur_song: m_playlist.cur

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
    property alias category: m_category
    property alias player: m_player
    property alias playlist: m_playlist
    property alias querier_song: m_querier_song

    property alias querier_user_song: m_querier_user_songlike
    property string song_cover: ''

    property int color_scheme: MD.MdColorMgr.Light
    property bool use_system_color_scheme: true
    property bool use_system_accent_color: true
    property color primary_color: MD.Token.color.accentColor

    property int cover_quality: -1

    readonly property string title: 'Qcm'

    readonly property QtObject user_info: root.userModel.activeUser
    readonly property string user_setting_category: 'user_test'//`user_${user_info.userId.sid}`

    readonly property alias user_song_set: m_querier_user_songlike.data

    signal sig_route(QA.RouteMsg msg)
    signal sig_route_special(string name)
    signal sig_popup_special(string name)

    copy_action_comp: Component {
        QA.CopyAction {
            icon.name: MD.Token.icon.title
            property string error
            getCopyString: function () {
                return error;
            }
        }
    }

    function join_name(objs, split) {
        const names = objs.map(o => {
            return o.name;
        });
        return names.join(split);
    }
    function route(dest, props = {}) {
        let url = dest;
        if (QA.App.isItemId(dest)) {
            url = QA.App.itemIdPageUrl(dest);
            props = {
                "itemId": dest
            };
        }
        if (QA.App.debug)
            console.error('route to:', url);
        sig_route_special('main');
        const msg = m_comp_route_msg.createObject(root, {
            "qml": url,
            "props": props
        });
        sig_route(msg);
        msg.destroy(3000);
    }
    function show_page_popup(url, props, popup_props = {}) {
        return MD.Util.show_popup('qrc:/Qcm/App/qml/component/PagePopup.qml', Object.assign({}, {
            "source": url,
            "props": props
        }, popup_props), root.main_win);
    }

    function appendList(songs_) {
        const songs = songs_.filter(s => {
            return s.canPlay;
        });
        const num = root.playlist.appendList(songs);
        root.toast(num ? qsTr(`Add ${num} songs to queue`) : qsTr('Already added'));
    }

    function toggleColorScheme() {
        color_scheme = color_scheme == MD.MdColorMgr.Dark ? MD.MdColorMgr.Light : MD.MdColorMgr.Dark;
    }

    Component {
        id: m_comp_route_msg
        QA.RouteMsg {}
    }

    LoggingCategory {
        id: m_category
        name: "qcm"
        defaultLogLevel: LoggingCategory.Warning
    }
    Settings {
        id: settings_audio
        property alias fade_time: m_player.fadeTime
        category: 'audio'
    }
    Settings {
        id: settings_play
        property alias loop: m_playlist.loopMode
        property alias volume: m_player.volume
        category: 'play'
    }
    Settings {
        id: settings_quality
        property alias cover_quality: root.cover_quality
        category: 'quality'
    }
    Settings {
        id: settings_theme
        property alias color_scheme: root.color_scheme
        property alias use_system_color_scheme: root.use_system_color_scheme
        property alias use_system_accent_color: root.use_system_accent_color
        property alias primary_color: root.primary_color
        category: 'theme'

        Component.onCompleted: {
            MD.Token.color.useSysColorSM = Qt.binding(() => {
                return root.use_system_color_scheme;
            });
            MD.Token.color.useSysAccentColor = Qt.binding(() => {
                return root.use_system_accent_color;
            });
            if (root.use_system_accent_color) {
                root.primary_color = MD.Token.color.accentColor;
            }
            MD.Token.color.accentColor = Qt.binding(() => {
                return primary_color;
            });
            if (MD.Token.color.useSysColorSM) {
                root.color_scheme = MD.Token.color.colorScheme;
            }
            MD.Token.color.colorScheme = Qt.binding(() => {
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
            if (status === QA.enums.Finished) {
                const song = songs.length ? songs[0] : null;
                const media_url = song ? QA.App.media_url(song.url, key) : '';
                m_player.source = media_url;
            } else if (status === QA.enums.Error) {
                m_player.stop();
            }
        }

        onCurChanged: function (refresh) {
            const song_url_sig = m_querier_song.statusChanged;
            if (song_url_slot)
                song_url_sig.disconnect(song_url_slot);
            if (!cur.itemId.valid()) {
                m_player.stop();
                return;
            }
            const quality = parseInt(settings_play.value('play_quality', m_querier_song.level.toString()));
            const key = Qt.md5(`${cur.itemId.sid}, quality: ${quality}`);
            const file = QA.App.media_file(key);
            // seems empty url is true, use string
            if (file.toString()) {
                if (refresh && root.player.source === file)
                    root.player.source = '';
                root.player.source = file;
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
    QNcm.SongLikeQuerier {
        id: m_querier_user_songlike
        function like_song(song_id, is_like) {
            const qu = m_querier_radio_like;
            qu.trackId = song_id;
            qu.like = is_like;
            qu.query();
        }

        autoReload: false
    }
    Connections {
        function onSongLiked(trackId, liked) {
            const qr = m_querier_user_songlike;
            if (liked)
                qr.data.insert(trackId);
            else
                qr.data.remove(trackId);
            qr.dataChanged();
        }
        target: QA.App
    }

    QNcm.RadioLikeQuerier {
        id: m_querier_radio_like
        autoReload: false

        onStatusChanged: {
            if (status === QA.enums.Finished) {
                QA.App.songLiked(trackId, like);
            }
        }
    }
    QNcm.SongUrlQuerier {
        id: m_querier_song
        autoReload: ids.length > 0
    }

    QA.Mpris {
        id: m_mpris
        player: root.player
        playlist: m_playlist
    }

    QA.QcmPlayer {
        id: m_player
        readonly property date durationDate: new Date(duration)
    }

    Connections {
        target: m_player
        function onSourceChanged() {
            if (m_player.source) {
                m_player.play();
            }
        }
        function onPlaybackStateChanged(old, new_) {
            const p = m_player;
            // console.debug(root.category, `state: ${p.playbackState}, ${p.position}, ${p.duration}, ${p.source}`);

            if (p.playbackState === QA.enums.StoppedState && p.source) {
                if (p.position / p.duration > 0.98) {
                    m_playlist.next();
                }
            }
            if (p.playbackState !== QA.enums.StoppedState) {
                root.playbackLog(m_player.playbackState, root.cur_song.itemId, root.cur_song.source?.itemId ?? QA.Util.create_itemid());
            }
        }
    }

    Connections {
        target: m_playlist

        property QA.t_song old
        function onCurChanged() {
            root.playbackLog(QA.enums.StoppedState, old.itemId, old.source?.itemId ?? QA.Util.create_itemid());
            old = m_playlist.cur;
        }
    }
}
