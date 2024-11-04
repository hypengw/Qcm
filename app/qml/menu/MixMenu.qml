import QtQuick
import QtQuick.Controls.Basic
import Qcm.App as QA
import Qcm.Material as MD

MD.Menu {
    id: root

    required property QA.t_id itemId 
    readonly property bool isUserPlaylist: QA.Global.session.user.userId === root.playlist.userId

    dim: false
    font.capitalization: Font.Capitalize
    modal: true

    QA.CommentAction {
        itemId: root.itemId
    }

    QA.CollectAction {
        itemId: root.itemId
    }

    Action {
        enabled: root.isUserPlaylist
        icon.name: MD.Token.icon.delete
        text: qsTr('delete')
        onTriggered: {
            qr_delete.itemIds = [root.itemId];
            qr_delete.query();
        }
    }

    ///QNcm.PlaylistDeleteQuerier {
    ///    id: qr_delete
    ///    autoReload: false
    ///    onStatusChanged: {
    ///        if (status === QA.enums.Finished)
    ///            QA.App.playqueueDeleted();
    ///    }
    ///}
}
