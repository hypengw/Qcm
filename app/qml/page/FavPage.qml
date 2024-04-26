import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    title: `add to playlist`
    padding: 0

    required property var songId

    MD.ListView {
        id: view
        anchors.fill: parent
        clip: true
        implicitHeight: contentHeight

        busy: qr_playlist.status === QA.qcm.Querying
        model: qr_playlist.data
        delegate: MD.ListItem {
            text: model.name
            supportText: `${model.trackCount} songs`
            width: ListView.view.width
            maximumLineCount: 2
            leader: MD.Image {
                radius: 8
                source: `image://ncm/${model.picUrl}`
                sourceSize.height: 48
                sourceSize.width: 48
            }
            onClicked: {
                qr_tracks.playlistId = model.itemId;
            }
        }

        QA.UserPlaylistQuerier {
            id: qr_playlist
            autoReload: uid.valid() && limit > 0
            uid: QA.Global.user_info.userId
            limit: 30
            Component.onCompleted: {
                data.onlyUserId = uid;
            }
        }

        QA.PlaylistTracksQuerier {
            id: qr_tracks
            trackIds: [root.songId]
            autoReload: playlistId.valid()
            onStatusChanged: {
                if (status === QA.qcm.Finished) {
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
