import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QcmApp
import ".."

ItemDelegate {
    id: root

    highlighted: ListView.isCurrentItem
    Material.foreground: highlighted ? Theme.color.getOn(background.color) : Theme.color.on_background

    Binding {
        target: root
        property: 'background.color'
        when: highlighted
        value: root.Material.primary
    }

}
