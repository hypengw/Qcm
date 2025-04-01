pragma Singleton
import QtCore
import QtQml
import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

QA.GlobalWrapper {
    id: root

    readonly property QA.t_song cur_song: QA.App.playqueue.currentSong

    readonly property string loop_icon: switch (QA.App.playqueue.loopMode) {
    case QA.enums.SingleLoop:
        return MD.Token.icon.repeat_one;
    case QA.enums.ListLoop:
        return MD.Token.icon.loop;
    case QA.enums.ShuffleLoop:
        return MD.Token.icon.shuffle;
    case QA.enums.NoneLoop:
    default:
        return MD.Token.icon.trending_flat;
    }

    property Window main_win: null
    property alias category: m_category
    property alias player: m_player

    property string song_cover: ''

    property int color_scheme: MD.MdColorMgr.Light
    property bool use_system_color_scheme: true
    property bool use_system_accent_color: true
    property color primary_color: MD.Token.color.accentColor

    property int cover_quality: -1
    readonly property string user_setting_category: 'user_test'//`user_${user_info.userId.sid}`

    copy_action_comp: Component {
        QA.CopyAction {
            icon.name: MD.Token.icon.title
            property string error
            getCopyString: function () {
                return error;
            }
        }
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

    QA.Mpris {
        id: m_mpris
        player: m_player
        playlist: QA.App.playqueue
    }

    QA.QcmPlayer {
        id: m_player
        readonly property date durationDate: new Date(duration)
    }

    Connections {
        target: QA.Action
        function onPlay(url, reload) {
            if (reload && m_player.source == Qt.url(url)) {
                m_player.source = '';
            }
            m_player.source = url;
            if (url) {
                m_player.play();
            }
        }
    }

    Connections {
        target: root
        function onSessionChanged() {
            m_player.stop();
            QA.App.playqueue.clear();
        }
    }

    Connections {
        target: m_player
        function onPlaybackStateChanged(old, new_) {
            const p = m_player;
            const queue = QA.App.playqueue;
            // console.debug(root.category, `state: ${p.playbackState}, ${p.position}, ${p.duration}, ${p.source}`);

            if (p.playbackState === QA.enums.StoppedState && p.source) {
                if (p.position / p.duration > 0.98) {
                    queue.next(queue.loopMode);
                }
            }
            if (p.playbackState !== QA.enums.StoppedState) {
                QA.Action.playbackLog(p.playbackState, root.cur_song.itemId, root.cur_song.source?.itemId ?? QA.Util.create_itemid());
            }
        }
    }
}
