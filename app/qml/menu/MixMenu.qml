import QtQuick
import QtQuick.Controls.Basic
import Qcm.App as QA
import Qcm.Material as MD

MD.Menu {
    id: root

    required property QA.item_id itemId
    required property QA.item_id userId
    readonly property bool isUserPlaylist: QA.Global.session.user.userId === root.userId

    dim: false
    font.capitalization: Font.Capitalize
    modal: true

    QA.CommentAction {
        itemId: root.itemId
    }

    QA.CollectAction {
        enabled: !root.isUserPlaylist
        itemId: root.itemId
    }

    Action {
        enabled: root.isUserPlaylist
        icon.name: MD.Token.icon.delete
        text: qsTr('delete')
        onTriggered: {
            m_query_delete.itemIds = [root.itemId];
            m_query_delete.reload();
        }
    }
    QA.MixDeleteQuery {
        id: m_query_delete
    }
}
