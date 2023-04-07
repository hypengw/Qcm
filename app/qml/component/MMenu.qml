import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import ".."

Menu {
    id: root
    Material.background: Theme.color.surface_container

    delegate: MMenuItem {
    }
}
