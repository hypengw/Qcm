import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.Menu {
    id: root

    required property QA.t_id itemId

    dim: false
    font.capitalization: Font.Capitalize
    modal: true


    QA.CollectAction {
        itemId: root.itemId
    }
}