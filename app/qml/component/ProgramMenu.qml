import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Menu {
    id: root

    required property QA.t_program program

    dim: false
    font.capitalization: Font.Capitalize
    modal: true

    Action {
        enabled: root.program.song.itemId !== QA.Global.cur_song.itemId
        icon.name: MD.Token.icon.play_arrow
        text: qsTr('play next')

        onTriggered: {
            QA.Global.playlist.appendNext(root.program.song);
        }
    }
    Action {
        icon.name: MD.Token.icon.comment
        text: qsTr('commnet')
        onTriggered: {
            QA.Global.show_page_popup('qrc:/Qcm/App/qml/page/CommentPage.qml', {
                    "itemId": root.program.itemId
                });
        }
    }
}