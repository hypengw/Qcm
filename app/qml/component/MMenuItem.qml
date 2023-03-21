import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import ".."

MenuItem {
    // mirrored: root.mirrored

    id: root

    font.pointSize: Theme.ts.label_large.size
    icon.width: 18
    icon.height: 18
    Material.foreground: {
        if (!enabled)
            return Material.hintTextColor;
        else
            return parent.Material.foreground;
    }

    contentItem: RowLayout {
        readonly property real arrowPadding: root.subMenu && root.arrow ? root.arrow.width + root.spacing : 0
        readonly property real indicatorPadding: root.checkable && root.indicator ? root.indicator.width + root.spacing : 0

        anchors.leftMargin: !root.mirrored ? indicatorPadding : arrowPadding
        anchors.rightMargin: root.mirrored ? indicatorPadding : arrowPadding
        spacing: root.spacing

        Label {
            visible: root.display !== AbstractButton.TextOnly && root.icon.name
            text: root.icon.name
            font.family: Theme.font.icon_round.family
            font.pointSize: Theme.ic_size(root.icon.width)
            color: Material.foreground
        }

        Label {
            visible: root.display !== AbstractButton.IconOnly
            text: root.text
            font: root.font
            color: Material.foreground
        }

        Item {
            Layout.fillWidth: true
        }

    }

}
