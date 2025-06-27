import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.Card {
    id: root

    property alias image: image
    property string subText
    property int picWidth: 160
    property MD.typescale typescale: MD.Token.typescale.body_medium

    horizontalPadding: 0
    bottomPadding: 8
    mdState.radius: MD.Token.shape.corner.small

    contentItem: Column {
        spacing: 8
        QA.Image {
            id: image
            width: displaySize.width
            height: displaySize.height
            displaySize: Qt.size(root.picWidth, root.picWidth)
            fixedSize: false
            radius: root.mdState.radius
        }

        Column {
            spacing: 4
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            anchors.left: parent.left
            anchors.right: parent.right

            MD.Label {
                id: label
                width: parent.width
                text: root.text
                maximumLineCount: 2
                typescale: root.typescale
            }

            MD.Label {
                id: label_sub
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.subText
                visible: !!text
                opacity: 0.6
                typescale: root.typescale
                maximumLineCount: 1
            }
        }
    }
}
