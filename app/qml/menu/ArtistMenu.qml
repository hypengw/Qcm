import QtQuick
import Qcm.App as QA
import Qcm.Material as MD
import Qcm.Msg as QM

MD.Menu {
    id: root

    required property QM.item_id itemId

    dim: false
    font.capitalization: Font.Capitalize
    modal: true

    QA.CollectAction {
        itemId: root.itemId
    }
}