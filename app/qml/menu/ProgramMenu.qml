import QtQuick
import Qcm.App as QA
import Qcm.Msg as QM
import Qcm.Material as MD

MD.Menu {
    id: root

    required property QA.item_id itemId
    required property QM.song song

    dim: false
    font.capitalization: Font.Capitalize
    modal: true

    QA.PlayNextAction {
        song: root.song
    }
    QA.CommentAction {
        itemId: root.itemId
    }
}