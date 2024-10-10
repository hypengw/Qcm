import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Page {
    id: root
    title: `add to playlist`
    padding: 0

    required property var songId

    MD.ListView {
        id: view
        anchors.fill: parent
        implicitHeight: contentHeight

        busy: qr_playlist.status === QA.enums.Querying
        model: qr_playlist.data
        delegate: MD.ListItem {
            text: model.name
            supportText: `${model.trackCount} songs`
            width: ListView.view.width
            maximumLineCount: 2
            leader: MD.Image {
                radius: 8
                source: QA.Util.image_url(model.picUrl)
                sourceSize.height: 48
                sourceSize.width: 48
            }
            onClicked: {
                qr_tracks.playlistId = model.itemId;
            }
        }

        QNcm.UserPlaylistQuerier {
            id: qr_playlist
            autoReload: uid.valid() && limit > 0
            uid: QA.Global.session.user.userId
            limit: 30
            Component.onCompleted: {
                data.onlyUserId = uid;
            }
        }

        QNcm.PlaylistTracksQuerier {
            id: qr_tracks
            trackIds: [root.songId]
            autoReload: playlistId.valid()
            onStatusChanged: {
                if (status === QA.enums.Finished) {
                    if (playlistId === view.model.item(0)?.itemId) {
                        QA.App.songLiked(root.songId, true);
                    }
                    QA.App.playlistChanged();
                    MD.Util.closePopup(root);
                }
            }
        }
    }
}
