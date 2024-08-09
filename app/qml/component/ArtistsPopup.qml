import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Page {
    id: root

    required property var model

    font.capitalization: Font.Capitalize
    title: qsTr('artists')
    width: 300

    MD.ListView {
        anchors.fill: parent
        currentIndex: -1
        implicitHeight: contentHeight
        model: root.model

        delegate: MD.ListItem {
            horizontalPadding: 8
            padding: 8
            width: ListView.view.width

            leader: MD.Image {
                source: `image://ncm/${modelData.picUrl}`
                sourceSize.height: 48
                sourceSize.width: 48
                radius: 8
            }
            text: modelData.name
            /*
            contentItem: RowLayout {
                // spacing: 12
                MD.Text {
                    Layout.fillWidth: true
                    text: modelData.name
                }
            }
            */

            onClicked: {
                MD.Util.closePopup(root);
                QA.Global.route(modelData.itemId);
            }
        }
    }
}
