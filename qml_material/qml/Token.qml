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

    property alias theme: m_color.colorScheme
    readonly property bool is_dark_theme: Number(theme) == MD.MdColorMgr.Dark

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

    readonly property QtObject typescale: MD.TypeScale {
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
