import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.Menu {
    id: root

    required property QA.item_id itemId

    dim: false
    font.capitalization: Font.Capitalize
    modal: true

    onItemIdChanged: {
        console.error(itemId)
    }

    QA.PlayAction {
        itemId: root.itemId
    }
    QA.FavoriteAction {
        itemId: root.itemId
    }

    // QA.CommentAction {
    //     itemId: root.itemId
    // }
}
