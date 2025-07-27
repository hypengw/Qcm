import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.Dialog {
    id: root
    property list<var> itemIds
    title: qsTr('go to artist')
    standardButtons: MD.Dialog.Cancel

   MD.VerticalListView {
        anchors.fill: parent
        model: root.itemIds

        expand: true

        delegate: MD.ListItem {
            required property var modelData
            model: null
            width: ListView.view.width
            corners: MD.Util.listCorners(index, count, 16)

            leader: QA.Image {
                source: QA.Util.image_url(m_qr.data.item.itemId)
                sourceSize.height: 48
                sourceSize.width: 48
                radius: 24
            }
            text: m_qr.data.item.name
            onImplicitWidthChanged: {
                const v = ListView.view;
                v.implicitWidth = Math.max(v.implicitWidth, implicitWidth);
            }

            QA.ArtistQuery {
                id: m_qr
                itemId: modelData
                onItemIdChanged: {
                    reload();
                }
            }
            onClicked: {
                MD.Util.closePopup(root);
                QA.Action.routeItem(modelData);
            }
        }
    }
}
