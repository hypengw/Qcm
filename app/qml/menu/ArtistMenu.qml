import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Menu {
    id: root

    required property QA.t_artist artist
    readonly property var itemId: artist.itemId

    dim: false
    font.capitalization: Font.Capitalize
    modal: true

    QA.SubAction {
        liked: root.artist.followed
        querier: qr_sub
        itemId: root.itemId
    }

    QNcm.ArtistSublistQuerier {
        id: qr_sub
        autoReload: false
        onStatusChanged: {
            if (status === QA.enums.Finished)
                QA.App.playqueueLiked(itemId, sub);
        }
    }
}