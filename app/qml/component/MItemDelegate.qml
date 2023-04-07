import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import ".."

ItemDelegate {
    id: root
    Material.foreground: highlighted ? Theme.color.getOn(Material.primary) : Theme.color.getOn(Material.background)
    Material.primary: Theme.color.secondary_container
    highlighted: {
        if (ListView)
            return ListView.isCurrentItem;
        else if (GridView)
            return GridView.isCurrentItem;
        return false;
    }

    Binding {
        property: 'background.color'
        target: root
        value: root.Material.primary
        when: highlighted
    }
}
