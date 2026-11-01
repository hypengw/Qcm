pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD
import Qcm.App as QA

MD.Menu {
    id: root
    title: qsTr("Display Mode")
    property int displayMode: 0
    property int gridSpacing: 12
    signal gridSpacingChangeRequested(int spacing)

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

    MD.MenuSeparator {}

    MD.Pane {
        implicitWidth: 240
        horizontalPadding: 16
        verticalPadding: 8
        background: null
        contentItem: Column {
            spacing: 8
            MD.Label {
                text: qsTr("Grid spacing: %1").arg(root.gridSpacing)
            }
            MD.Slider {
                objectName: "gridSpacingSlider"
                width: parent.width
                enabled: root.displayMode !== QA.Enum.DList
                from: 0
                to: 16
                stepSize: 1
                snapMode: MD.SliderBase.SnapAlways
                value: root.gridSpacing
                Accessible.name: qsTr("Grid spacing")
                onValueChangeRequested: requestedValue => root.gridSpacingChangeRequested(Math.round(requestedValue))
            }
        }
    }
}
