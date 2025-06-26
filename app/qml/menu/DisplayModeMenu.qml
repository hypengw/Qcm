import QtQuick
import Qcm.Material as MD

MD.Menu {
    id: root
    title: qsTr("Display Mode")

    component ListItem: MD.ListItem {
        model: null
        index: -1
        width: parent.width
        mdState.backgroundColor: mdState.ctx.color.surface_container
    }

    contentItem: Column {
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width
        spacing: 2

        ListItem {
            text: qsTr("List")
            icon.name: MD.Token.icon.list_alt
        }

        ListItem {
            text: qsTr("Grid")
            icon.name: MD.Token.icon.grid_view
        }

        ListItem {
            text: qsTr("Card Grid")
            icon.name: MD.Token.icon.grid_view
        }
    }
}
