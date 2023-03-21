import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QcmApp
import ".."

Popup {
    id: root

    default property alias m_content: colu.children
    property string title
    property alias titleCapitalization: label_title.font.capitalization
    readonly property int leftHeight: height - head_pane.implicitHeight
    readonly property int maxHeight: parent.height * 0.8

    parent: Overlay.overlay
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    width: parent.width / 2
    height: maxHeight
    modal: true
    padding: 0

    ColumnLayout {
        id: colu

        anchors.fill: parent
        spacing: 0

        Pane {
            id: head_pane

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            Material.background: Theme.color.surface_1
            Material.elevation: 1
            padding: 8

            RowLayout {
                anchors.fill: parent

                Label {
                    id: label_title

                    Layout.fillWidth: true
                    text: title
                    font.pointSize: 14
                    font.capitalization: Font.Capitalize
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
