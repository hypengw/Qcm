import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

import QcmApp

Pane {
    id: root
    padding: 4
    horizontalPadding: 8
    Material.foreground: Theme.color.on_primary

    Binding on implicitHeight {
        when: root.contentItem.implicitHeight < 42
        value: 42
    }

    Binding {
        target: root
        property: 'background.color'
        value: root.Material.primary
    }
}
