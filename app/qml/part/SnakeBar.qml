import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import ".."
import "../component"

Pane {
    id: root

    property alias text: label.text
    property Action action: null
    property bool showClose: false

    padding: 0
    leftPadding: 16
    rightPadding: showClose ? 8 : 16
    height: 48
    Material.elevation: 3
    Material.background: Theme.color.inverse_surface
    Material.foreground: Theme.color.inverse_on_surface

    RowLayout {
        id: icon_layout

        anchors.fill: parent
        spacing: 8

        Label {
            id: label

            Layout.fillWidth: true
            font.pointSize: Theme.ts.body_medium.size
        }

        MButton {
            Material.theme: Theme.toMatTheme(Theme.theme, true)
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

        MRoundButton {
            visible: root.showClose
            Material.theme: Theme.toMatTheme(Theme.theme, true)
            text: Theme.ic.close
            flat: true
        }

    }

}
