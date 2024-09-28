import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Menu {
    id: root

    required property QA.t_playlist playlist
    readonly property var itemId: playlist.itemId
    readonly property bool isUserPlaylist: QA.Global.session.user.userId === root.playlist.userId

    dim: false
    font.capitalization: Font.Capitalize
    modal: true

    QNcm.CommentAction {
        itemId: root.itemId
    }

    QA.SubAction {
        enabled: !root.isUserPlaylist
        liked: root.playlist.subscribed
        querier: qr_sub
        itemId: root.itemId
    }

    QNcm.PlaylistSubscribeQuerier {
        id: qr_sub
        autoReload: false
        onStatusChanged: {
            if (status === QA.enums.Finished)
                QA.App.playlistLiked(itemId, sub);
        }
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

    QNcm.PlaylistDeleteQuerier {
        id: qr_delete
        autoReload: false
        onStatusChanged: {
            if (status === QA.enums.Finished)
                QA.App.playlistDeleted();
        }
    }
}
