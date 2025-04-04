import QtQuick
import QtQuick.Controls.Basic

import Qcm.App as QA
import Qcm.Material as MD

Action {
    id: root
    property QA.t_id songId: song.itemId
    property QA.song song

    enabled: root.itemId !== QA.App.playqueue.currentSong.itemId
    icon.name: MD.Token.icon.play_arrow
    text: qsTr('play next')

    onTriggered: {
        if(song.itemId.valid()) {
            QA.Action.play(root.song);
        } else {
            QA.Action.play_by_id(root.songId);
        }
    }
}
