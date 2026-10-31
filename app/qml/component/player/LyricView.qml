pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD
import Qcm.App as QA

MD.VerticalLazyList {
    id: root
    property QA.LyricModel model
    source: QA.ItemModelSource {
        model: root.model
    }
    spacing: 4
    extraTopMargin: height / 2
    extraBottomMargin: height / 2
    footer: null

    function posTo(index) {
        if (visible && count > 0 && !dragging)
            positionAtIndex(Math.max(0, index), 0.5, 0, MD.LazyList.Smooth);
    }
    function syncPosition() { posTo(model?.currentIndex ?? -1); }
    onCountChanged: Qt.callLater(syncPosition)
    onVisibleChanged: if (visible) Qt.callLater(syncPosition)
    onWidthChanged: Qt.callLater(syncPosition)
    onHeightChanged: Qt.callLater(syncPosition)

    delegate: MD.ItemDelegate {
        id: lyricItem
        required property MD.LazyRow row
        readonly property var entry: row.value
        readonly property bool current: root.model?.currentIndex === row.index
        contentItem: MD.Text {
            typescale: {
                const ts = MD.Token.typescale.title_large.fresh();
                ts.weight = Font.DemiBold;
                return ts;
            }
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: lyricItem.entry.content
            color: root.MD.MProp.textColor
            maximumLineCount: -1
            opacity: lyricItem.current ? 1 : 0.8
        }
        onClicked: QA.Global.player.position = entry.milliseconds
    }
}
