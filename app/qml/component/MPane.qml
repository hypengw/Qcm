import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Pane {
    id: root

    property real radius: 0

    Binding {
        target: root
        property: 'background.radius'
        value: root.radius
    }

}
