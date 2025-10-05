pragma Singleton
import QtCore
import QtQml
import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

QA.GlobalWrapper {
    id: root

    readonly property QA.song cur_song: QA.App.playqueue.currentSong

    property Window main_win: null
    property alias category: m_category

    property string song_cover: ''

    property int color_scheme: MD.Enum.Light
    property bool use_system_color_scheme: true
    property bool use_system_accent_color: true
    property color primary_color: MD.Token.color.accentColor
    property int palette_type: MD.Token.color.paletteType

    property color playing_color: m_picker.color

    property int cover_quality: -1

    toastActionComp: Component {
        QA.CopyAction {
            icon.name: MD.Token.icon.title
            property string error
            getCopyString: function () {
                return error;
            }
        }
    }

    function toggleColorScheme() {
        color_scheme = color_scheme == MD.Enum.Dark ? MD.Enum.Light : MD.Enum.Dark;
    }

    LoggingCategory {
        id: m_category
        name: "qcm"
        defaultLogLevel: LoggingCategory.Warning
    }

    Settings {
        id: settings_theme
        property alias color_scheme: root.color_scheme
        property alias use_system_color_scheme: root.use_system_color_scheme
        property alias use_system_accent_color: root.use_system_accent_color
        property alias primary_color: root.primary_color
        property alias palette_type: root.palette_type
        category: 'theme'

        Component.onCompleted: {
            MD.Token.color.accentColor = Qt.binding(() => {
                return root.primary_color;
            });
            MD.Token.themeMode = Qt.binding(() => {
                return root.color_scheme;
            });
            MD.Token.color.paletteType = Qt.binding(() => {
                return root.palette_type;
            });
            MD.Token.color.useSysAccentColor = Qt.binding(() => {
                return root.use_system_accent_color;
            });
            MD.Token.color.useSysColorSM = Qt.binding(() => {
                return root.use_system_color_scheme;
            });
        }
    }

    QA.Mpris {
        id: m_mpris
        player: root.player
        playlist: QA.App.playqueue
    }

    MD.ColorPicker {
        id: m_picker
    }

    Connections {
        target: QA.Notifier
        function onSpecialImageLoaded(name, img) {
            m_picker.image = img;
            m_picker.pick();
        }
    }
}
