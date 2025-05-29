pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD
import Qcm.App as QA

MD.VerticalListView {
    id: root

    function posTo(idx) {
        if (!visible)
            return;
        if (idx === -1) {
            anim_scroll.manual_stop();
            positionViewAtIndex(0, ListView.Center);
            forceLayout();
        } else {
            anim_scroll.from = contentY;
            positionViewAtIndex(idx, ListView.Center);
            anim_scroll.to = contentY;
            contentY = anim_scroll.from;
            anim_scroll.running = !moving;
        }
    // seem when not visible, this is not accurate
    // use a timer to sync
    }
    function timer_restart() {
        if (visible)
            timer_scroll.restart();
    }

    highlightFollowsCurrentItem: false
    highlightRangeMode: ListView.NoHighlightRange
    reuseItems: true
    spacing: 4

    SmoothedAnimation {//on contentY {
        id: anim_scroll
        target: root
        property: 'contentY'
        running: false

        property bool manual_stopped: false

        function manual_stop() {
            manual_stopped = true;
            stop();
        }

        duration: 1000
        velocity: 200

        onStopped: {
            return;
            if (manual_stopped) {
                manual_stopped = false;
                return;
            }

            if (root.count === 0)
                return;

            const cur = root.itemAtIndex(root.model.currentIndex);
            if (cur) {
                const center = cur.y + cur.height / 2;
                const list_center = root.contentY + root.height / 2;
                const diff = Math.abs(center - list_center);
                if (diff > 10) {
                    timer_scroll.triggered();
                }
            } else {
                root.timer_restart();
            }
        }
    }
    delegate: MD.ListItem {
        required property int index
        required property var model
        readonly property bool current: root.model.currentIndex === index
        width: ListView.view.width

        contentItem: ColumnLayout {
            MD.Text {
                Layout.fillWidth: true
                typescale: {
                    const ts = MD.Token.typescale.title_large.fresh();
                    ts.weight = Font.DemiBold;
                    return ts;
                }
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: model.content
                color: current ? MD.Token.color.primary : MD.Token.color.on_surface
                maximumLineCount: -1
            }
        }

        onClicked: {
            QA.Global.player.position = model.milliseconds;
        }
    }
    footer: Item {
        height: ListView.view.height / 2
    }
    header: Item {
        height: ListView.view.height / 2
    }

    onHeightChanged: timer_restart()
    onMovingChanged: {
        if (moving)
            anim_scroll.manual_stop();
    }
    onVisibleChanged: timer_restart()
    // onWheelMoved: anim_scroll.manual_stop()
    onWidthChanged: timer_restart()

    Timer {
        id: timer_scroll
        interval: 400
        repeat: false
        running: true

        onTriggered: {
            const idx = root.model.currentIndex;
            root.model.currentIndexChanged(idx);
        }
    }
}
