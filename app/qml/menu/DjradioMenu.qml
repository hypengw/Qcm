import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Menu {
    id: root

    required property QA.t_djradio djradio
    readonly property var itemId: djradio.itemId

    dim: false
    font.capitalization: Font.Capitalize
    modal: true

    QNcm.CommentAction {
        itemId: root.itemId
    }

    QA.SubAction {
        liked: root.djradio.subscribed
        querier: qr_sub
        itemId: root.itemId
    }

    QNcm.DjradioSublistQuerier {
        id: qr_sub
        autoReload: false
        onStatusChanged: {
            if (status === QA.enums.Finished)
                QA.App.playlistLiked(itemId, sub);
        }
    }
}