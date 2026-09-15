import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.ListGridBaseDelegate {
    id: root

    cellHeight: widthProvider.width + 72

    property alias mdState: m_card.mdState
    property alias subText: m_card.subText

    QA.ImageCard {
        id: m_card
        x: parent.cellX
        y: parent.cellY
        width: parent.widthProvider.width

        image.source: QA.Util.image_url(parent.model.itemId)
        text: parent.model.name
        subText: QA.Util.joinName(QA.Store.extra(root.model.itemId)?.artists, "/") || qsTr("%1 tracks").arg(root.model.trackCount)
        picWidth: width
        onClicked: root.clicked()
    }
}
