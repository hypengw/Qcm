import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.ListGridBaseDelegate {
    id: root

    cellHeight: widthProvider.width + 72 + (hasSubText ? (m_card.labelSpacing + m_card.subImplicitHeight) : 0)

    property alias mdState: m_card.mdState
    property alias subText: m_card.subText
    property bool hasSubText: false

    QA.ImageCard {
        id: m_card
        x: parent.cellX
        y: parent.cellY
        width: parent.widthProvider.width

        image.source: QA.Util.image_url(parent.model.itemId)
        text: parent.model.name
        picWidth: width
        onClicked: root.clicked()
    }
}
