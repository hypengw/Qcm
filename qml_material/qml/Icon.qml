import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD

Item {
    id: root

    implicitWidth: size
    implicitHeight: size

    property string name
    property int size
    property alias horizontalAlignment: item_text_icon.horizontalAlignment

    property int lineHeight: MD.Token.typescale.label_large.line_height
    property int iconStyle: MD.Enum.IconRound

    Text {
        id: item_text_icon
        anchors.centerIn: parent

        font.family: {
            switch (root.iconStyle) {
            case MD.Enum.IconRound:
            default:
                return MD.Token.font.icon_round.family;
            }
        }

        font.pixelSize: root.size
        text: root.name
        color: MD.MatProp.textColor
        lineHeight: root.lineHeight
        lineHeightMode: Text.FixedHeight
    }
}
