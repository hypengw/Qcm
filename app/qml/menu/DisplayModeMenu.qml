pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD
import Qcm.App as QA

MD.Menu {
    id: root
    title: qsTr("Display Mode")
    property int displayMode: 0

    component ListItem: MD.MenuItem {
        property int value
        selected: value == root.displayMode
        onClicked: root.displayMode = value
    }

    ListItem {
        text: qsTr("List")
        value: QA.Enum.DList
        icon.name: MD.Token.icon.list_alt
    }

    ListItem {
        text: qsTr("Grid")
        value: QA.Enum.DGrid
        icon.name: MD.Token.icon.grid_view
    }

    ListItem {
        text: qsTr("Card Grid")
        value: QA.Enum.DCardGrid
        icon.name: MD.Token.icon.grid_view
    }
}
