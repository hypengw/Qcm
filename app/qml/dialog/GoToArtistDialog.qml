import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.Dialog {
    id: root
    property list<var> itemIds
    title: qsTr('go to artist')
    standardButtons: MD.Dialog.Cancel

    MD.ListView {
        anchors.fill: parent
        model: root.itemIds
        expand: true

        delegate: MD.ListItem {
            required property var modelData
            required property int index
            width: ListView.view.width
            corners: indexCorners(index, count, 16)

            leader: QA.Image {
                source: QA.Util.image_url(m_qr.data.info.picUrl)
                sourceSize.height: 48
                sourceSize.width: 48
                radius: 24
            }
            text: m_qr.data.info.name
            onImplicitWidthChanged: {
                const v = ListView.view;
                v.implicitWidth = Math.max(v.implicitWidth, implicitWidth);
            }

            QA.ArtistDetailQuery {
                id: m_qr
                delay: false
                itemId: modelData
            }
            onClicked: {
                MD.Util.closePopup(root);
                QA.Action.route_by_id(modelData);
            }
        }
    }
}
