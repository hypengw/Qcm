import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    property alias text: label.text
    title: qsTr('description')

    MD.Flickable {
        id: flick

        anchors.fill: parent
        leftMargin: 24
        rightMargin: 24

        ColumnLayout {
            id: content

            height: implicitHeight
            width: parent.width
            spacing: 12

            MD.Text {
                id: label
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                maximumLineCount: -1
                typescale: MD.Token.typescale.body_medium
            }
        }
    }
    footer: MD.Control {
        horizontalPadding: 24
        contentItem: RowLayout {
            Item {
                Layout.fillWidth: true
            }
            MD.Button {
                type: MD.Enum.BtText
                text: qsTr('close')
                onClicked: {
                    MD.Util.closePopup(root);
                }
            }
        }
    }
}
