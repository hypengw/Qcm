import QtQuick
import Qcm.Material as MD

MD.Pane {
    id: root

    required property string tag
    MD.MatProp.textColor: MD.Token.color.tertiary
    height: tag_label.lineHeight
    padding: 0
    horizontalPadding: 2

    MD.Text {
        id: tag_label
        text: root.tag
        font.capitalization: Font.AllUppercase
        typescale: MD.Token.typescale.label_small
        verticalAlignment: Qt.AlignVCenter
    }

    background: Rectangle {
        color: 'transparent'
        implicitHeight: 0
        radius: 1
        border.width: 1
        border.color: root.MD.MatProp.textColor
    }

}
