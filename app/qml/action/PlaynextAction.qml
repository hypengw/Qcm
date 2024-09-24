import QtQuick
import QtQuick.Controls

import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

Action {
    id: root
    required property QA.t_song song
    enabled: song.itemId !== QA.Global.cur_song.itemId
    icon.name: MD.Token.icon.play_arrow
    text: qsTr('play next')

    onTriggered: {
        console.error(root.song);
        QA.App.playlist.appendNext(root.song);
    }
}
