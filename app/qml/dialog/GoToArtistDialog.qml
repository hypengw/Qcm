import QtQuick
import Qcm.App as QA
import Qcm.Msg as QM
import Qcm.Material as MD

MD.Dialog {
    id: root
    property QM.artistIds artistIds
    property bool albumArtist: false
    title: qsTr('Go To Artist')
    standardButtons: MD.Dialog.Cancel

    MD.VerticalLazyList {
        id: m_view
        anchors.fill: parent
        source: MD.ListSnapshotSource {
            items: QM.Ids.artistItems(root.artistIds)
        }

        expand: true

        delegate: MD.ListItem {
            required property MD.LazyRow row
            itemRow: row
            readonly property var modelData: row.value
            model: null

            corners: MD.Util.listCorners(index, count, 16)

            leader: QA.Image {
                source: QA.Util.image_url(modelData)
                displaySize: Qt.size(48, 48)
                radius: 24
            }
            text: m_qr.data.item.name
            onImplicitWidthChanged: {
                const v = m_view;
                v.implicitWidth = Math.max(v.implicitWidth, implicitWidth);
            }

            QA.ArtistQuery {
                id: m_qr
                artistId: modelData
                onArtistIdChanged: {
                    reload();
                }
            }
            onClicked: {
                MD.Util.closePopup(root);
                QA.Action.routeItem(modelData, {"albumArtist": root.albumArtist});
            }
        }
    }
}
