import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.Menu {
    id: root

    required property QA.t_id itemId
    required property QA.t_song song

    dim: false
    font.capitalization: Font.Capitalize
    modal: true

    QA.PlaynextAction {
        song: root.song
    }
    QA.CommentAction {
        itemId: root.itemId
    }
}