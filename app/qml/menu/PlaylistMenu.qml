import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Menu {
    id: root

    required property QA.t_playlist playlist
    readonly property var itemId: playlist.itemId

    dim: false
    font.capitalization: Font.Capitalize
    modal: true

    QA.CommentAction {
        itemId: root.itemId
    }

    QA.SubAction {
        enabled: QA.Global.user_info.userId !== root.playlist.userId
        liked: root.playlist.subscribed
        querier: qr_sub
        itemId: root.itemId
    }

    QA.PlaylistSubscribeQuerier {
        id: qr_sub
        autoReload: false
        onStatusChanged: {
            if (status === QA.ApiQuerierBase.Finished)
                QA.App.playlistLiked(itemId, sub);
        }
    }
}
