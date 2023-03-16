import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import ".."

RoundButton {
    id: root

    Material.foreground: highlighted ? Theme.color.getOn(Material.accent) : Theme.color.on_background

    Binding {
        target: root
        property: 'contentItem.color'
        when: highlighted
        value: root.Material.foreground
    }

}
