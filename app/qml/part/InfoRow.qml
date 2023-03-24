import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import "../component"

Pane {
    id: root

    property alias icon_name: icon_row.text
    property alias label_text: label.text

    padding: 0
    horizontalPadding: 8

    IconRowLayout {
        id: icon_row

        anchors.fill: parent
        iconSize: 16

        Label {
            id: label

            Layout.fillWidth: true
            Layout.preferredHeight: Material.buttonHeight
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

    }

}
