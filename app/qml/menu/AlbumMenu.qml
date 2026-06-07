import QtQuick
import Qcm.App as QA
import Qcm.Material as MD
import Qcm.Msg as QM

MD.Menu {
    id: root
    required property QA.item_id itemId
    font.capitalization: Font.Capitalize
    dim: false
    modal: true

    QA.PlayAction {
        itemId: root.itemId
        closeMenu: false
    }

    QA.FavoriteAction {
        id: m_favorite_action
        itemId: root.itemId
    }

    QA.AddToMixAction {
        oper: QM.MixManipulateOper.MIX_MANIPULATE_OPER_ADD_ALBUMS
        albumIds: [root.itemId]
    }

    // QA.CommentAction {
    //     itemId: root.itemId
    // }
}
