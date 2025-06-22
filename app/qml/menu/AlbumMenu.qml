import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

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

    // QA.CommentAction {
    //     itemId: root.itemId
    // }
}
