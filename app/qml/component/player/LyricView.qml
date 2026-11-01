pragma ComponentBehavior: Bound
import QtQuick
import Qcm.Material as MD
import Qcm.App as QA

MD.VerticalLazyList {
    id: root
    property QA.LyricModel model
    property bool __userScrolling: false
    property bool __immediatePosition: true
    source: QA.ItemModelSource {
        model: root.model
        onCommitted: root.schedulePosition(true)
    }
    spacing: 4
    extraTopMargin: height / 2
    extraBottomMargin: height / 2
    footer: null

    function posTo(index, mode = MD.LazyList.Smooth) {
        if (visible && width > 0 && height > 0 && count > 0 && !__userScrolling && !dragging)
            positionAtIndex(Math.max(0, index), 0.5, 0, mode);
    }
    function schedulePosition(immediate) {
        __immediatePosition = __immediatePosition || immediate;
        Qt.callLater(syncPosition);
    }
    function syncPosition() {
        if (__userScrolling || dragging)
            return;
        const mode = __immediatePosition ? MD.LazyList.Immediate : MD.LazyList.Smooth;
        __immediatePosition = false;
        posTo(model?.currentIndex ?? -1, mode);
    }
    onCountChanged: schedulePosition(true)
    onModelChanged: schedulePosition(true)
    onVisibleChanged: if (visible) schedulePosition(true)
    onWidthChanged: schedulePosition(true)
    onHeightChanged: schedulePosition(true)

    MD.NestedScroll.onScrollStarted: __userScrolling = true
    MD.NestedScroll.onScrollFinished: {
        __userScrolling = false;
        schedulePosition(false);
    }
    Connections {
        target: root.model
        function onCurrentIndexChanged() { root.schedulePosition(false); }
    }

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
