import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Qcm.Material as MD

MD.Pane {
    id: root

    required property string tag

    MD.MatProp.textColor: MD.Token.color.tertiary
    padding: 1
    horizontalPadding: 2

    MD.Text {
        id: tag_label
        text: root.tag
        font.capitalization: Font.AllUppercase
        typescale: MD.Token.typescale.label_medium
    }

    background: Rectangle {
        color: 'transparent'
        radius: 1
        border.width: 1
        border.color: root.MD.MatProp.textColor
    }

}
