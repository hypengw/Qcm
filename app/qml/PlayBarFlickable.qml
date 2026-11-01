pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Window
import Qcm.App as QA
import Qcm.Material as MD

MD.Scrollable {
    id: root
    objectName: "playbackHost"

    property rect closedBarRect: Qt.rect(0, height - barHeight, width, barHeight)
    property real barMargin: 0
    property Item backdropSource
    property bool floating: true
    property bool hasSong: QA.App.playqueue.currentSongId.valid
    property bool changingTrack: QA.Global.player.changingTrack
    property bool presentationRequested: false
    property real presentationProgress: presentationRequested ? 1 : 0
    readonly property real presentationOffset: (1 - presentationProgress) * Math.max(0, height - closedBarRect.y + barMargin)
    readonly property real barHeight: bar.implicitHeight
    readonly property real travel: Math.max(0, closedBarRect.y)
    readonly property real expansion: travel > 0 ? Math.max(0, Math.min(1, contentY / travel)) : 0
    readonly property bool closed: !presentationRequested || (playbackState === "close" && !motion.running && expansion === 0)
    readonly property bool expanded: presentationRequested && playbackState === "open" && !motion.running
    property real previousTravel: 0
    property bool ready: false
    property Item returnFocusItem

    visible: presentationProgress > 0
    enabled: presentationRequested && presentationProgress > 0
    contentHeight: height + travel
    contentWidth: width
    flickableDirection: MD.Scrollable.VerticalFlick
    property string playbackState: "close"
    property string releaseState: ""
    containmentMask: QtObject {
        function contains(point: point): bool {
            if (!root.presentationRequested || root.presentationProgress <= 0)
                return false;
            if (!root.closed)
                return point.x >= 0 && point.y >= 0 && point.x < root.width && point.y < root.height;
            const rect = root.closedBarRect;
            const y = rect.y + root.presentationOffset;
            return point.x >= rect.x && point.y >= y && point.x < rect.x + rect.width && point.y < y + rect.height;
        }
    }
    MD.InputBlock {
        target: root
        acceptWheel: false
        acceptHover: true
        acceptTouch: true
        acceptMouseButtons: Qt.LeftButton
    }

    function syncPresentation() {
        if (!ready)
            return;
        if (hasSong)
            presentationRequested = true;
        else if (!changingTrack)
            presentationRequested = false;
    }
    onHasSongChanged: syncPresentation()
    onChangingTrackChanged: syncPresentation()
    onPresentationRequestedChanged: {
        if (!ready || presentationRequested)
            return;
        releaseState = "";
        cancelFlick();
        if (playbackState === "close")
            settle();
        else
            playbackState = "close";
    }
    Behavior on presentationProgress {
        enabled: root.ready
        NumberAnimation {
            duration: root.MD.MProp.motion.reducedMotion ? 0 : Math.round(300 * root.MD.MProp.motion.durationScale)
            easing.type: Easing.OutCubic
        }
    }

    function rememberFocus() {
        if (playbackState === "close" && !motion.running)
            returnFocusItem = root.Window.window?.activeFocusItem ?? null;
    }
    onExpandedChanged: if (expanded)
        page.forceActiveFocus(Qt.OtherFocusReason)
    onClosedChanged: {
        if (closed && returnFocusItem) {
            if (returnFocusItem.visible && returnFocusItem.enabled)
                returnFocusItem.forceActiveFocus(Qt.OtherFocusReason);
            returnFocusItem = null;
        }
    }
    function settle() {
        if (!ready || playbackState === "drag")
            return;
        motion.stop();
        motion.from = contentY;
        motion.to = playbackState === "open" ? travel : 0;
        if (Math.abs(motion.from - motion.to) < 0.5)
            contentY = motion.to;
        else
            motion.start();
    }
    function chooseState() {
        if (!presentationRequested || playbackState !== "drag")
            return;
        const open = flicking ? verticalVelocity > 0 : expansion > 0.5;
        playbackState = open ? "open" : "close";
    }
    function back() {
        if (expanded && page.canBack)
            page.back();
        else
            playbackState = "close";
    }
    onMovementStarted: {
        if (!presentationRequested || releaseState.length) return;
        rememberFocus();
        motion.stop();
        playbackState = "drag";
    }
    onDragReleased: velocity => {
        if (!presentationRequested) return;
        releaseState = Math.abs(velocity.y) >= 75 ? (velocity.y > 0 ? "open" : "close")
                                                : (expansion > 0.5 ? "open" : "close");
        // Snap after the scroll coordinator has finished releasing its grab.
        Qt.callLater(finishRelease);
    }
    function finishRelease() {
        if (!presentationRequested || !releaseState.length) return;
        const target = releaseState;
        cancelFlick();
        releaseState = "";
        playbackState = target;
        settle();
    }
    onMovementEnded: if (!releaseState.length) chooseState()
    onPlaybackStateChanged: settle()
    onTravelChanged: {
        if (!ready)
            return;
        const fraction = previousTravel > 0 ? contentY / previousTravel : (playbackState === "open" ? 1 : 0);
        motion.stop();
        previousTravel = travel;
        contentY = Math.max(0, Math.min(1, fraction)) * travel;
        settle();
    }
    onFloatingChanged: {
        cancelFlick();
        if (playbackState === "drag")
            playbackState = expansion > 0.5 ? "open" : "close";
    }
    Component.onCompleted: {
        previousTravel = travel;
        ready = true;
        syncPresentation();
        settle();
    }
    NumberAnimation {
        id: motion
        target: root
        property: "contentY"
        duration: root.MD.MProp.motion.reducedMotion ? 0 : Math.round(300 * root.MD.MProp.motion.durationScale)
        easing.type: Easing.OutCubic
    }
    Shortcut {
        sequences: ["Space"]
        enabled: root.visible && root.presentationRequested
        onActivated: QA.Action.toggle()
    }
    Connections {
        target: QA.Action
        function onTogglePlaybar() {
            if (root.presentationRequested && root.playbackState !== "drag") {
                root.rememberFocus();
                root.playbackState = root.playbackState === "close" ? "open" : "close";
            }
        }
    }
    QA.PlayingPage {
        id: page
        objectName: "playingPage"
        y: root.travel
        width: root.width
        height: root.height
        visible: root.expansion > 0 && root.presentationProgress > 0
        enabled: root.expanded
        opacity: root.expansion * root.presentationProgress
    }
    QA.PlayBar {
        id: bar
        x: root.closedBarRect.x * (1 - root.expansion)
        y: root.travel + root.presentationOffset
        width: root.closedBarRect.width + (root.width - root.closedBarRect.width) * root.expansion
        height: implicitHeight
        layoutWidth: Math.max(0, root.width - 2 * root.barMargin)
        floating: root.floating
        backdropSource: root.backdropSource
        visible: root.expansion < 0.5
        opacity: root.presentationProgress * Math.max(1 - root.expansion * 2, 0)
    }
}
