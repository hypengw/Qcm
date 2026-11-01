pragma Singleton
import QtCore
import QtQml
import QtQuick
import Qcm.App as QA
import Qcm.Msg as QM
import Qcm.Material as MD
import QExtra as QE

QA.GlobalWrapper {
    id: root

    property Window main_win: null
    property alias category: m_category

    property string song_cover: ''

    property int color_scheme: MD.Enum.Light
    property bool use_system_color_scheme: true
    property bool use_system_accent_color: true
    property color primary_color: MD.Token.color.accentColor
    property int palette_type: MD.Token.color.paletteType
    property int playbar_background_style: QA.PlayBarBackground.PageBlur

    property int cover_quality: -1
    property int image_decode_cache_mib: 16
    property int image_gpu_cache_mib: 32

    function applyImageTextureCache() {
        if (main_win)
            QE.ImageCache.setTextureCapacityMiB(main_win, image_gpu_cache_mib);
    }

    onMain_winChanged: applyImageTextureCache()
    onImage_gpu_cache_mibChanged: applyImageTextureCache()

    Binding {
        target: QE.ImageCache
        property: "capacityMiB"
        value: root.image_decode_cache_mib
    }

    Settings {
        category: "image_cache"
        property alias decode_capacity_mib: root.image_decode_cache_mib
        property alias gpu_capacity_mib: root.image_gpu_cache_mib
        Component.onCompleted: {
            root.image_decode_cache_mib = Math.max(0, Math.min(128, root.image_decode_cache_mib));
            root.image_gpu_cache_mib = Math.max(0, Math.min(256, root.image_gpu_cache_mib));
        }
    }

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
        property alias playbar_background_style: root.playbar_background_style
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
}
