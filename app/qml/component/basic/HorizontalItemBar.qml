import QtQuick
import Qcm.Material as MD

MD.Control {
    id: root

    property string title
    property MD.LazyList view

    contentItem: Item {
        implicitWidth: m_title.implicitWidth + m_tool_row.implicitWidth
        implicitHeight: Math.max(m_title.implicitHeight, m_tool_row.implicitHeight)
        MD.Text {
            id: m_title
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            text: root.title
            typescale: MD.Token.typescale.headline_medium
        }
        Row {
            id: m_tool_row
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            MD.IconButton {
                objectName: "previousItems"
                icon.name: MD.Token.icon.chevron_left
                enabled: root.view ? !root.view.atXBeginning : false
                onClicked: {
                    if (!root.view)
                        return;
                    root.view.positionAtIndex(Math.max(0, root.view.firstVisibleIndex - 4), 0, 0, MD.LazyList.Smooth);
                }
            }
            MD.IconButton {
                objectName: "nextItems"
                icon.name: MD.Token.icon.chevron_right
                enabled: root.view ? !root.view.atXEnd : false
                onClicked: {
                    if (!root.view)
                        return;
                    root.view.positionAtIndex(Math.min(root.view.count - 1, Math.max(0, root.view.firstVisibleIndex) + 4), 0, 0, MD.LazyList.Smooth);
                }
            }
        }
    }
}
