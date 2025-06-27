pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD
import Qcm.App as QA

MD.Menu {
    id: root
    title: qsTr("Display Mode")
    property int displayMode: 0

    MD.ButtonGroup {
        id: m_group
        onClicked: btn => {
            root.displayMode = (btn as ListItem)?.value;
        }
    }
    component ListItem: MD.MenuItem {
        property int value
        width: parent.width
        checked: value == root.displayMode
        selected: checked
    }

    ListItem {
        text: qsTr("List")
        value: QA.Enum.DList
        icon.name: MD.Token.icon.list_alt
        MD.ButtonGroup.group: m_group
    }

    ListItem {
        text: qsTr("Grid")
        value: QA.Enum.DGrid
        icon.name: MD.Token.icon.grid_view
        MD.ButtonGroup.group: m_group
    }

    ListItem {
        text: qsTr("Card Grid")
        value: QA.Enum.DCardGrid
        icon.name: MD.Token.icon.grid_view
        MD.ButtonGroup.group: m_group
    }
}
