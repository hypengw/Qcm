pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Window
import Qcm.App as QA
import Qcm.Material as MD

MD.SliderBase {
    id: root

    property var player: QA.Global.player
    property var queue: QA.App.playqueue
    property bool wavy: false
    property MD.StateSliderM2 mdState: MD.StateSliderM2 {
        item: root
    }
    readonly property url songIdentity: queue?.currentSongId?.url ?? ""
    readonly property double playing_pos: Number.isFinite(player.duration) && player.duration > 0 && Number.isFinite(player.position) ? Math.max(0, Math.min(1, player.position / player.duration)) : 0
    readonly property bool seekVisible: visible && (Window.window?.visible ?? false) && Window.window.visibility !== Window.Minimized
    readonly property bool interacting: interaction.pressed || interaction.dragged
    property int generation: 0
    property bool ready: false

    implicitWidth: 200
    implicitHeight: 48
    enabled: player.seekable && Number.isFinite(player.duration) && player.duration > 0
    live: true
    padding: 0
    to: 1
    Accessible.role: Accessible.Slider
    Accessible.name: qsTr("Playback position")
    Accessible.onIncreaseAction: increase()
    Accessible.onDecreaseAction: decrease()

    MD.InteractionObserver {
        id: interaction
        source: root.interactionSource
    }

    function syncPosition() {
        if (!interacting && slider_timer.pos < 0 && !block_timer.blocked && !player.busy)
            value = playing_pos;
    }
    function discardSeek() {
        slider_timer.stop();
        slider_timer.pos = -1;
        block_timer.stop();
        block_timer.blocked = false;
    }
    function cancelSeek() {
        if (!ready)
            return;
        ++generation;
        cancelInteraction();
        discardSeek();
        value = playing_pos;
    }
    onPlaying_posChanged: syncPosition()
    onSongIdentityChanged: cancelSeek()
    onPlayerChanged: cancelSeek()
    onQueueChanged: cancelSeek()
    onSeekVisibleChanged: if (!seekVisible)
        cancelSeek()
    onEnabledChanged: if (!enabled)
        cancelSeek()
    Component.onCompleted: {
        ready = true;
        syncPosition();
    }
    Component.onDestruction: discardSeek()

    Connections {
        target: root.player
        function onSourceChanged() {
            root.cancelSeek();
        }
        function onBusyChanged() {
            root.syncPosition();
        }
    }

    onValueChangeRequested: requestedValue => {
        if (!Number.isFinite(requestedValue))
            return;
        value = requestedValue;
        slider_timer.pos = requestedValue;
        slider_timer.generation = root.generation;
        slider_timer.restart();
    }
    onInteractionFinished: cancelled => {
        slider_timer.stop();
        if (cancelled) {
            discardSeek();
            value = playing_pos;
        } else {
            slider_timer.submit();
        }
    }

    Timer {
        id: slider_timer
        property double pos: -1
        property int generation: -1
        interval: 500
        function submit() {
            const position = pos;
            pos = -1;
            if (position < 0 || generation !== root.generation || !root.seekVisible || !root.enabled)
                return;
            block_timer.blocked = true;
            block_timer.restart();
            root.player.seek(position);
        }
        onTriggered: submit()
    }
    Timer {
        id: block_timer
        interval: 300
        property bool blocked: false
        onTriggered: {
            blocked = false;
            root.syncPosition();
        }
    }

    handle: MD.SliderHandleM2 {
        x: root.leftPadding + root.visualPosition * (root.availableWidth - width)
        y: root.topPadding + (root.availableHeight - height) / 2
        value: root.previewValue
        handleHasFocus: root.focusVisible
        handlePressed: root.interacting
        handleHovered: interaction.hovered
    }
    backgroundLayout: MD.Control.LayoutNone
    background: MD.SquigglyProgress {
        id: track
        x: root.leftPadding + (root.handle?.width ?? 0) / 2
        y: root.topPadding + (root.availableHeight - height) / 2
        width: Math.max(0, root.availableWidth - (root.handle?.width ?? 0))
        height: 12
        progress: root.position
        animate: root.wavy && root.player.playing && !root.interacting
        animationsEnabled: !root.MD.MProp.motion.reducedMotion && root.MD.MProp.motion.durationScale > 0
        waveAmplitude: root.wavy ? 1.5 : 0
        transitionEnabled: false
        strokeWidth: 3
        bufferedRange: root.player.cacheProgress
        color: root.MD.MProp.color.primary
        trackColor: root.MD.MProp.color.surface_container_highest
        bufferedColor: root.MD.MProp.color.primary_container
        transform: Scale {
            origin.x: track.width / 2
            xScale: root.mirrored ? -1 : 1
        }
        Accessible.ignored: true
    }
    MD.FocusIndicator {
        anchors.fill: parent
        corners: MD.Util.corners(8)
        inset: true
        active: root.focusVisible
    }
}
