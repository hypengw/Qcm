import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.Card {
    id: root

    property alias image: image
    property string subText
    property int picWidth: 160
    property MD.typescale typescale: MD.Token.typescale.body_medium
    property int labelSpacing: 4
    property bool mipmap: false
    readonly property int subImplicitHeight: label_sub.implicitHeight

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
            mipmap: root.mipmap
        }

        Column {
            spacing: root.labelSpacing
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            anchors.left: parent.left
            anchors.right: parent.right

            MD.Label {
                id: label
                width: parent.width
                text: root.text
                maximumLineCount: 1
                wrapMode: Text.NoWrap
                horizontalAlignment: Text.AlignLeft
                typescale: root.typescale
            }

            MD.Label {
                id: label_sub
                text: root.subText
                width: parent.width
                visible: !!text
                opacity: 0.6
                typescale: MD.Token.typescale.body_small
                maximumLineCount: 1
                wrapMode: Text.NoWrap
                horizontalAlignment: Text.AlignLeft
            }
        }
    }
}
