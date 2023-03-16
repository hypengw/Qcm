import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import ".."

Pane {
    id: root

    required property string tag
    property alias pointSize: tag_label.font.pointSize

    Material.foreground: Theme.color.tertiary
    padding: 1
    horizontalPadding: 2

    Label {
        id: tag_label

        text: root.tag
        font.capitalization: Font.AllUppercase
        font.bold: true
    }

    background: Rectangle {
        color: 'transparent'
        radius: 1
        border.width: 1
        border.color: Material.foreground
    }

}
