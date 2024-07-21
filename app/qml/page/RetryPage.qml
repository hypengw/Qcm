import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Page {
    id: root
    padding: 0

    property alias text: item_text.text
    property var retryCallback: function () {}

    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter

        MD.Text {
            id: item_text
            Layout.alignment: Qt.AlignHCenter
            MD.MatProp.textColor: MD.Token.color.error
            typescale: MD.Token.typescale.label_large
        }

        MD.Button {
            Layout.alignment: Qt.AlignHCenter
            type: MD.Enum.BtText
            action: Action {
                icon.name: MD.Token.icon.refresh
                text: 'retry'
                onTriggered: root.retryCallback()
            }
        }
    }

    MD.IconButton {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.bottomMargin: 16

        action: Action {
            icon.name: MD.Token.icon.settings

            onTriggered: {
                QA.Global.show_page_popup('qrc:/Qcm/App/qml/page/SettingsPage.qml', {});
            }
        }
    }
}
