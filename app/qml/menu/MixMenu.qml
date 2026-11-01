import QtQuick
import Qcm.App as QA
import Qcm.Material as MD
import Qcm.Msg as QM

MD.Menu {
    id: root

    required property QA.item_id itemId

    dim: false
    font.capitalization: Font.Capitalize
    modal: true

    QA.MixDeleteAction {
        ids: QM.Ids.mixIds([root.itemId])
    }
    QA.CopyMenu {
        itemId: root.itemId
    }
    QA.CommentAction {
        itemId: root.itemId
    }
}
