import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.Menu {
    id: root

    required property QA.item_id itemId
    required property QA.song song

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