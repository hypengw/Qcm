pragma Singleton
import QtCore
import QtQml
import QtQuick
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

QA.GlobalWrapper {
    id: root

    readonly property QA.t_song cur_song: QA.App.playlist.cur

    readonly property string loop_icon: switch (QA.App.playlist.loopMode) {
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
    property var playlist: QA.App.playlist
    property alias querier_song: m_querier_song

    property alias querier_user_song: m_querier_user_songlike
    property string song_cover: ''

    property int color_scheme: MD.MdColorMgr.Light
    property bool use_system_color_scheme: true
    property bool use_system_accent_color: true
    property color primary_color: MD.Token.color.accentColor

    property int cover_quality: -1
    readonly property string user_setting_category: 'user_test'//`user_${user_info.userId.sid}`

    readonly property alias user_song_set: m_querier_user_songlike.data

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
        QA.Action.route_special('main');
        const msg = QA.Util.create_route_msg({
            "url": url,
            "props": props
        });
        QA.Action.route(msg);
    }

    function toggleColorScheme() {
        color_scheme = color_scheme == MD.MdColorMgr.Dark ? MD.MdColorMgr.Light : MD.MdColorMgr.Dark;
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
    Connections {
        target: QA.App.playlist
        property var song_url_slot: null

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

        function onCurChanged(refresh) {
            const p = QA.App.playlist;
            const song_url_sig = m_querier_song.statusChanged;
            if (song_url_slot)
                song_url_sig.disconnect(song_url_slot);
            if (!p.cur.itemId.valid()) {
                m_player.stop();
                return;
            }
            const quality = parseInt(settings_play.value('play_quality', m_querier_song.level.toString()));
            const key = Qt.md5(`${p.cur.itemId.sid}, quality: ${quality}`);
            const file = QA.Util.media_cache_of(key);
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
                const songId = p.cur.itemId;
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
        playlist: root.playlist
    }

    QA.QcmPlayer {
        id: m_player
        readonly property date durationDate: new Date(duration)
    }

    Connections {
        target: root
        function onSessionChanged() {
            m_player.stop();
            root.playlist.clear();

            if(root.session.valid) {
                m_querier_user_songlike.query();
            }
        }
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
                    root.playlist.next();
                }
            }
            if (p.playbackState !== QA.enums.StoppedState) {
                QA.Action.playbackLog(m_player.playbackState, root.cur_song.itemId, root.cur_song.source?.itemId ?? QA.Util.create_itemid());
            }
        }
    }
}
