import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.Card {
    id: root

    property MD.LazyRow itemRow
    property alias image: image
    property string subText
    property real picWidth: 160
    property MD.typescale typescale: MD.Token.typescale.body_medium
    property int labelSpacing: QA.ImageCardMetrics.labelSpacing
    property real labelLineHeight: 0
    property bool mipmap: false
    readonly property int subImplicitHeight: label_sub.implicitHeight

    horizontalPadding: 0
    bottomPadding: QA.ImageCardMetrics.bottomPadding
    mdState.radius: MD.Token.shape.corner.small

    contentItem: Column {
        spacing: QA.ImageCardMetrics.imageSpacing
        QA.Image {
            id: image
            itemRow: root.itemRow
            width: displaySize.width
            height: displaySize.height
            displaySize: Qt.size(root.picWidth, root.picWidth)
            fixedSize: false
            radius: root.mdState.radius
            mipmap: root.mipmap
        }

        Column {
            spacing: root.labelSpacing
            height: root.labelLineHeight > 0 ? root.labelLineHeight * 2 + spacing : implicitHeight
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            anchors.left: parent.left
            anchors.right: parent.right

            MD.Label {
                id: label
                width: parent.width
                height: root.labelLineHeight > 0 ? root.labelLineHeight : implicitHeight
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
                height: root.labelLineHeight > 0 ? root.labelLineHeight : implicitHeight
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
