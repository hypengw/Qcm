import QtQuick
import QtQuick.Controls.Basic

import Qcm.App as QA
import Qcm.Material as MD

Action {
    id: root
    required property QA.t_song song
    enabled: song.itemId !== QA.Global.cur_song.itemId
    icon.name: MD.Token.icon.play_arrow
    text: qsTr('play next')

    onTriggered: {
        QA.App.playqueue.appendNext(root.song);
    }
}
