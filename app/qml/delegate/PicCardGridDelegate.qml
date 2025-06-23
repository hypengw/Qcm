import QtQuick
import Qcm.App as QA

Item {
    id: root

    property alias text: m_card.text
    property alias subText: m_card.subText
    property alias picWidth: m_card.picWidth
    property alias image: m_card.image

    signal clicked

    width: GridView.view.cellWidth
    height: GridView.view.cellHeight
    implicitHeight: children[0].implicitHeight

    QA.ImageCard {
        id: m_card
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.GridView.view.fixedCellWidth
        picWidth: parent.GridView.view.fixedCellWidth
    }

    Connections {
        target: m_card
        function onClicked() {
            root.clicked();
        }
    }
}
