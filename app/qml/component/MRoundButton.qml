import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import ".."

RoundButton {
    id: root

    Material.foreground: {
        if (!enabled)
            return Material.hintTextColor;
        else if ((flat && highlighted) || (checked && !highlighted))
            return Material.accentColor;
        else if (highlighted)
            return Theme.color.getOn(Material.accentColor);
        else
            return parent.Material.foreground;
    }

    contentItem: Label {
        visible: root.display !== AbstractButton.TextOnly
        text: root.icon.name || root.text
        font.family: Theme.font.icon_round.family
        font.pointSize: Theme.ic_size(root.icon.width)
        color: Material.foreground
    }

}
