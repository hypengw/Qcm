import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import ".."

Slider {
    id: root

    readonly property double playing_pos: QA.player.duration > 0 ? QA.player.position / QA.player.duration : 0

    background.implicitHeight: 8
    live: false
    padding: 0
    to: 1

    onPlaying_posChanged: {
        if (!pressed)
            value = playing_pos;
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
                QA.player.seek(pos);
                pos = -1;
            }
        }
    }
}
