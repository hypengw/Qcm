import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import ".."

ItemDelegate {
    id: root

    highlighted: {
        if (ListView)
            return ListView.isCurrentItem;
        else if (GridView)
            return GridView.isCurrentItem;
        return false;
    }
    Material.foreground: highlighted ? Theme.color.getOn(background.color) : Theme.color.on_background

    Binding {
        target: root
        property: 'background.color'
        when: highlighted
        value: root.Material.primary
    }

}
