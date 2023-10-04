pragma Singleton
import QtCore
import QtQml
import QtQuick
import Qcm.Material as MD

Item {
    readonly property alias color: m_color
    // typescale
    readonly property QtObject font: QtObject {
        readonly property var default_font: Qt.application.font
        readonly property FontLoader fontload_material_outline: FontLoader {
            source: 'qrc:/Qcm/Material/assets/MaterialIconsOutlined-Regular.otf'
        }
        readonly property FontLoader fontload_material_round: FontLoader {
            source: 'qrc:/Qcm/Material/assets/MaterialIconsRound-Regular.otf'
        }
        readonly property var icon_outline: this.fontload_material_outline.font
        readonly property var icon_round: this.fontload_material_round.font
    }
    readonly property var icon: MD.IconToken.codeMap

    readonly property QtObject ic: QtObject {
        readonly property string add: '\ue145'
        readonly property string album: '\ue019'
        readonly property string arrow_back: '\ue5c4'
        readonly property string close: '\ue5cd'
        readonly property string content_copy: '\ue14d'
        readonly property string dark_mode: '\ue51c'
        readonly property string done: '\ue876'
        readonly property string edit: '\ue3c9'
        readonly property string equalizer: '\ue01d'
        readonly property string favorite: '\ue87d'
        readonly property string favorite_border: '\ue87e'
        readonly property string info: '\ue88e'
        readonly property string library_music: '\ue030'
        readonly property string light_mode: '\ue518'
        readonly property string link: '\ue157'
        readonly property string loop: '\ue028'
        readonly property string menu: '\ue5d2'
        readonly property string more_vert: '\ue5d4'
        readonly property string music_note: '\ue405'
        readonly property string pause: '\ue034'
        readonly property string person: '\ue7fd'
        readonly property string play_arrow: '\ue037'
        readonly property string playlist_add: '\ue03b'
        readonly property string playlist_play: '\ue05f'
        readonly property string queue_music: '\ue03d'
        readonly property string remove: '\ue15b'
        readonly property string repeat_one: '\ue041'
        readonly property string settings: '\ue8b8'
        readonly property string shuffle: '\ue043'
        readonly property string skip_next: '\ue044'
        readonly property string skip_previous: '\ue045'
        readonly property string today: '\ue8df'
        readonly property string trending_flat: '\ue8e4'
    }
    readonly property bool is_dark_theme: theme === MD.MdColorMgr.Dark
    property alias theme: m_color.schemeTheme

    //  Font.Thin	0
    //  Font.ExtraLight	12
    //  Font.Light	25
    //  Font.Normal	50
    //  Font.Medium	57
    //  Font.DemiBold	63
    //  Font.Bold	75
    //  Font.ExtraBold	81
    //  Font.Black	87

    // Value 	Common weight name
    // 100 	Thin (Hairline)
    // 200 	Extra Light (Ultra Light)
    // 300 	Light
    // 400 	Normal (Regular)
    // 500 	Medium
    // 600 	Semi Bold (Demi Bold)
    // 700 	Bold
    // 800 	Extra Bold (Ultra Bold)
    // 900 	Black (Heavy)
    // 950 	Extra Black (Ultra Black)

    readonly property QtObject typescale: QtObject {
        readonly property QtObject display_large: QtObject {
            readonly property int size: 57
            readonly property int line_height: 64
            readonly property int weight: Font.Normal
            readonly property real tracking: -0.25
        }
        readonly property QtObject display_medium: QtObject {
            readonly property int size: 45
            readonly property int line_height: 52
            readonly property int weight: Font.Normal
            readonly property real tracking: 0
        }
        readonly property QtObject display_small: QtObject {
            readonly property int size: 36
            readonly property int line_height: 44
            readonly property int weight: Font.Normal
            readonly property real tracking: 0
        }
        readonly property QtObject headline_large: QtObject {
            readonly property int size: 32
            readonly property int line_height: 40
            readonly property int weight: Font.Normal
            readonly property real tracking: 0
        }
        readonly property QtObject headline_medium: QtObject {
            readonly property int size: 28
            readonly property int line_height: 36
            readonly property int weight: Font.Medium
            readonly property real tracking: 0
        }
        readonly property QtObject headline_small: QtObject {
            readonly property int size: 24
            readonly property int line_height: 32
            readonly property int weight: Font.Normal
            readonly property real tracking: 0
        }
        readonly property QtObject title_large: QtObject {
            readonly property int size: 22
            readonly property int line_height: 28
            readonly property int weight: Font.Normal
            readonly property real tracking: 0
        }
        readonly property QtObject title_medium: QtObject {
            readonly property int size: 16
            readonly property int line_height: 24
            readonly property int weight: Font.Medium
            readonly property real tracking: 0.15
        }
        readonly property QtObject title_small: QtObject {
            readonly property int size: 14
            readonly property int line_height: 20
            readonly property int weight: Font.Medium
            readonly property real tracking: 0.1
        }
        readonly property QtObject label_large: QtObject {
            readonly property int size: 14
            readonly property int line_height: 20
            readonly property int weight: Font.Medium
            readonly property int weight_prominent: Font.Bold
            readonly property real tracking: 0.1
        }
        readonly property QtObject label_medium: QtObject {
            readonly property int size: 12
            readonly property int line_height: 16
            readonly property int weight: Font.Medium
            readonly property int weight_prominent: Font.Bold
            readonly property real tracking: 0.5
        }
        readonly property QtObject label_small: QtObject {
            readonly property int size: 11
            readonly property int line_height: 16
            readonly property int weight: Font.Medium
            readonly property int weight_prominent: Font.Bold
            readonly property real tracking: 0.5
        }
        readonly property QtObject body_large: QtObject {
            readonly property int size: 16
            readonly property int line_height: 24
            readonly property int weight: Font.Normal
            readonly property real tracking: 0.5
        }
        readonly property QtObject body_medium: QtObject {
            readonly property int size: 14
            readonly property int line_height: 20
            readonly property int weight: Font.Normal
            readonly property real tracking: 0.25
        }
        readonly property QtObject body_small: QtObject {
            readonly property int size: 12
            readonly property int line_height: 16
            readonly property int weight: Font.Normal
            readonly property real tracking: 0.4
        }
    }

    readonly property QtObject state: QtObject {
        readonly property QtObject hover: QtObject {
            readonly property real state_layer_opacity: 0.08
        }
        readonly property QtObject pressed: QtObject {
            readonly property real state_layer_opacity: 0.12
        }
    }
    readonly property QtObject elevation: QtObject {
        readonly property int level0: 0
        readonly property int level1: 1
        readonly property int level2: 3
        readonly property int level3: 6
        readonly property int level4: 8
        readonly property int level5: 12
    }

    readonly property QtObject shape: QtObject {
        readonly property QtObject corner: QtObject {
            readonly property int extra_large: 28
            readonly property int extra_small: 4
        }
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

    function button_color() {
    }

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

    MD.MdColorMgr {
        id: m_color
    }
}
