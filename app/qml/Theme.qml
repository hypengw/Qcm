pragma Singleton
import QtQml
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import Qt.labs.settings
import QcmApp

Item {
    // typescale

    readonly property var
    font: QtObject {
        readonly property var icon_round: this.fontload_material_round.font
        readonly property var icon_outline: this.fontload_material_outline.font
        readonly property var default_font: Qt.application.font
        readonly property var label_font: this.label_item.font
        readonly property int w_unit: this.text_item.implicitWidth
        readonly property var
        text_item: Text {
            text: '0'
        }

        readonly property var
        label_item: Label {
            text: '0'
        }

        readonly property var
        fontload_material_round: FontLoader {
            source: 'qrc:/QcmApp/assets/MaterialIconsRound-Regular.otf'
        }

        readonly property var
        fontload_material_outline: FontLoader {
            source: 'qrc:/QcmApp/assets/MaterialIconsOutlined-Regular.otf'
        }

        function small(s) {
            return s.pointSize * 0.8;
        }

    }

    readonly property QtObject
    ic: QtObject {
        readonly property string settings: '\ue8b8'
        readonly property string more_vert: '\ue5d4'
        readonly property string today: '\ue8df'
        readonly property string info: '\ue88e'
        readonly property string person: '\ue7fd'
        readonly property string album: '\ue019'
        readonly property string loop: '\ue028'
        readonly property string repeat_one: '\ue041'
        readonly property string shuffle: '\ue043'
        readonly property string trending_flat: '\ue8e4'
        readonly property string library_music: '\ue030'
        readonly property string play_arrow: '\ue037'
        readonly property string pause: '\ue034'
        readonly property string playlist_add: '\ue03b'
        readonly property string queue_music: '\ue03d'
        readonly property string skip_next: '\ue044'
        readonly property string skip_previous: '\ue045'
        readonly property string light_mode: '\ue518'
        readonly property string dark_mode: '\ue51c'
        readonly property string menu: '\ue5d2'
        readonly property string add: '\ue145'
        readonly property string remove: '\ue15b'
        readonly property string music_note: '\ue405'
        readonly property string close: '\ue5cd'
        readonly property string done: '\ue876'
        readonly property string arrow_back: '\ue5c4'
        readonly property string equalizer: '\ue01d'
        readonly property string favorite: '\ue87d'
        readonly property string favorite_border: '\ue87e'
        readonly property string edit: '\ue3c9'
        readonly property string playlist_play: '\ue05f'
    }

    readonly property var ts: ({
        "title_small": {
            "size": 12
        },
        "title_medium": {
            "size": 14
        },
        "title_large": {
            "size": 20
        },
        "label_small": {
            "size": 8.5
        },
        "label_medium": {
            "size": 10.5
        },
        "label_large": {
            "size": 12.5
        },
        "body_small": {
            "size": 10
        },
        "body_medium": {
            "size": 12
        },
        "body_large": {
            "size": 14
        }
    })
    readonly property alias color: m_color
    readonly property alias theme: m_color.schemeTheme
    readonly property bool is_dark_theme: theme === MdColorMgr.Dark

    // seems icon font size need map
    function ic_size(s) {
        switch (Math.floor(s)) {
        case 24:
            return 18;
        default:
            return s;
        }
    }

    function toMatTheme(th, inverse = false) {
        function fn_inverse(v, iv) {
            return iv ? !v : v;
        }

        return fn_inverse(th === MdColorMgr.Dark, inverse) ? Material.Dark : Material.Light;
    }

    MdColorMgr {
        id: m_color
    }

    Settings {
        property alias color_scheme: m_color.schemeTheme

        category: 'theme'
    }

}
