import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Menu {
    id: root

    required property QA.t_album album
    readonly property var itemId: album.itemId

    dim: false
    font.capitalization: Font.Capitalize
    modal: true

    QNcm.CommentAction {
        itemId: root.itemId
    }

    QA.SubAction {
        liked: root.album.subscribed
        querier: qr_sub
        itemId: root.itemId
    }

    QNcm.AlbumSubQuerier {
        id: qr_sub
        autoReload: false
        onStatusChanged: {
            if (status === QA.enums.Finished)
                QA.App.albumLiked(itemId, sub);
        }
    }
}