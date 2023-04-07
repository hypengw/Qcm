import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import ".."

Popup {
    id: root

    readonly property int contentMaxHeight: Math.max(maxHeight - headHeight, 0)
    readonly property int headHeight: head_pane.implicitHeight
    default property alias m_content: colu.children
    readonly property int maxHeight: parent.height * 0.8
    property string title
    property alias titleCapitalization: label_title.font.capitalization

    Material.background: Theme.color.surface_container_high
    font.pointSize: Theme.ts.label_medium.size
    height: Math.min(implicitHeight, maxHeight)
    modal: true
    padding: 0
    parent: Overlay.overlay
    width: parent.width / 2
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)

    ColumnLayout {
        id: colu
        anchors.fill: parent
        spacing: 0

        Pane {
            id: head_pane
            Layout.alignment: Qt.AlignTop
            Layout.fillWidth: true
            Material.elevation: 1
            padding: 8
            z: 1

            RowLayout {
                anchors.fill: parent

                Label {
                    id: label_title
                    Layout.fillWidth: true
                    font.capitalization: Font.Capitalize
                    font.pointSize: 14
                    text: title
                }
                MRoundButton {
                    flat: true
                    icon.name: Theme.ic.close

                    onClicked: {
                        root.close();
                    }
                }
            }
        }
    }
}
