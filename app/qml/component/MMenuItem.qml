import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import ".."

MenuItem {
    // mirrored: root.mirrored
    id: root
    Material.foreground: {
        if (!enabled)
            return Material.hintTextColor;
        else
            return parent.Material.foreground;
    }
    font.pointSize: Theme.ts.label_large.size
    icon.height: 18
    icon.width: 18

    contentItem: RowLayout {
        readonly property real arrowPadding: root.subMenu && root.arrow ? root.arrow.width + root.spacing : 0
        readonly property real indicatorPadding: root.checkable && root.indicator ? root.indicator.width + root.spacing : 0

        anchors.leftMargin: !root.mirrored ? indicatorPadding : arrowPadding
        anchors.rightMargin: root.mirrored ? indicatorPadding : arrowPadding
        spacing: root.spacing

        Label {
            color: Theme.color.on_surface_variant
            font.family: Theme.font.icon_round.family
            font.pointSize: Theme.ic_size(root.icon.width)
            opacity: enabled ? 1 : 0.38
            text: root.icon.name
            visible: root.display !== AbstractButton.TextOnly && root.icon.name
        }
        Label {
            color: Theme.color.on_surface
            font: root.font
            opacity: enabled ? 1 : 0.38
            text: root.text
            visible: root.display !== AbstractButton.IconOnly
        }
        Item {
            Layout.fillWidth: true
        }
    }
}
