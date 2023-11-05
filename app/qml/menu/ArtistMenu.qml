import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
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

    QA.ArtistSublistQuerier {
        id: qr_sub
        autoReload: false
        onStatusChanged: {
            if (status === QA.ApiQuerierBase.Finished)
                QA.App.playlistLiked(itemId, sub);
        }
    }
}