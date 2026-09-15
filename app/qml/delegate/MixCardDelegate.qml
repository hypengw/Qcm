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
        subText: qsTr("%1 songs").arg(root.model.trackCount)
        picWidth: width
        onClicked: root.clicked()
    }
}
