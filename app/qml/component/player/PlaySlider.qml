import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.SliderM2 {
    id: root

    property double playing_pos: 0
    readonly property bool interacting: interaction.pressed || interaction.dragged

    MD.InteractionObserver {
        id: interaction
        source: root.interactionSource
    }

    Binding {
        when: QA.Global.player.duration > 0 && !QA.Global.player.busy
        root.playing_pos: QA.Global.player.position / QA.Global.player.duration
        restoreMode: Binding.RestoreNone
    }

    enabled: QA.Global.player.seekable
    live: true
    padding: 0
    to: 1

    onPlaying_posChanged: {
        if (!interacting && !block_timer.blocked)
            value = playing_pos;
    }
    onValueChangeRequested: requestedValue => {
        value = requestedValue;
        slider_timer.recordPos(requestedValue);
    }
    onInteractionFinished: cancelled => {
        slider_timer.stop();
        if (cancelled) {
            slider_timer.pos = -1;
            value = playing_pos;
        } else {
            slider_timer.submit();
        }
    }

    Timer {
        id: slider_timer

        property double pos: -1

        function recordPos(pos_) {
            pos = pos_;
            restart();
        }

        interval: 500

        function submit() {
            if (pos >= 0) {
                const requestedPosition = pos;
                pos = -1;
                block_timer.block();
                QA.Global.player.seek(requestedPosition);
            }
        }
        onTriggered: submit()
    }

    Timer {
        id: block_timer
        interval: 300
        property bool blocked: false
        function block() {
            blocked = true;
            restart();
        }
        onTriggered: {
            blocked = false;
        }
    }

    overlay: QA.Global.player.cacheProgress
}
