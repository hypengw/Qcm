import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Qcm.Material as MD

MD.Pane {
    id: root

    property alias text: label.text
    property Action action: null
    property bool showClose: false

    padding: 0
    leftPadding: 16
    rightPadding: showClose ? 8 : 16
    height: 48
    MD.MatProp.elevation: MD.Token.elevation.level3

    RowLayout {
        id: icon_layout

        anchors.fill: parent
        spacing: 8

        MD.Text {
            id: label

            Layout.fillWidth: true
            typescale: MD.Token.typescale.body_medium
        }

/*
        MD.Button {
            Material.foreground: Theme.color.inverse_primary
            padding: 4
            horizontalPadding: 8
            leftInset: 0
            rightInset: 0
            topInset: 0
            bottomInset: 0
            background.implicitWidth: 0
            background.implicitHeight: 0
            flat: true
            font.capitalization: Font.Capitalize
            visible: !!action
            action: root.action
        }
*/

        MD.Icon {
            visible: root.showClose
            name: Theme.ic.close
            size: 24
        }

    }

}
