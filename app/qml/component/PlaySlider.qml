import QtQuick
import QtQuick.Controls
import Qcm.App as QA
import Qcm.Material as MD

MD.Slider {
    id: root

    property double playing_pos: 0
    property bool in_anim: false

    Binding {
        when: QA.Global.player.duration > 0
        restoreMode: Binding.RestoreNone
        root.playing_pos: QA.Global.player.position / QA.Global.player.duration
    }

    live: false
    padding: 0
    to: 1

    /*
    NumberAnimation on value  {
        id: anim_v
    }

    onPlaying_posChanged: {
        if (!anim_v.running && in_anim)
            in_anim = false;
        if (!pressed) {
            const max_dur = 1000.0;
            const pre = 200.0;
            const duration = QA.player.duration;
            const to = playing_pos + pre / duration;
            const pos = position;
            {
                anim_v.from = pos;
                anim_v.to = to;
                anim_v.duration = duration * Math.abs(playing_pos - pos) + pre;
                if (anim_v.duration > max_dur) {
                    const x = anim_v.duration - max_dur;
                    anim_v.duration = Math.max(max_dur - x, pre);
                    if (!in_anim) {
                        anim_v.restart();
                        in_anim = true;
                    }
                }
            }
            if (!in_anim)
                anim_v.restart();
        }
    }
    */
    onPlaying_posChanged: {
        if (!pressed && !block_timer.blocked) {
            const to = playing_pos;
            value = to;
        }
    }
    onPositionChanged: {
        if (pressed)
            slider_timer.recordPos(position);
    }
    onPressedChanged: {
        if (!pressed) {
            slider_timer.stop();
            slider_timer.triggered();
        }
    }

    Timer {
        id: slider_timer

        property double pos

        function recordPos(pos_) {
            pos = pos_;
            restart();
        }

        interval: 500

        onTriggered: {
            if (pos > 0) {
                QA.Global.player.seek(pos);
                pos = -1;
                block_timer.block();
            }
        }
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
}
