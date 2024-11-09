import QtQuick
import QtQuick.Controls.Basic
import Qcm.App as QA
import Qcm.Material as MD

MD.Menu {
    id: root

    property QA.t_id itemId
    property var song: qr_detail.data
    readonly property QA.t_id itemId_: itemId.valid() ? itemId : song.itemId

    dim: false
    font.capitalization: Font.Capitalize
    modal: true

    QA.PlaynextAction {
        enabled: root.itemId_ !== QA.App.playqueue.currentSong.itemId
        songId: root.itemId
    }
    Action {
        icon.name: MD.Token.icon.queue
        text: qsTr('Add to Playlist')
        onTriggered: {
            QA.Action.popup_page('qrc:/Qcm/App/qml/page/FavPage.qml', {
                songId: root.itemId_
            });
        }
    }

    Action {
        icon.name: MD.Token.icon.album
        text: qsTr('go to album')

        onTriggered: {
            QA.Global.route(root.song.album.itemId);
        }
    }
    QA.GoToArtistAction {
        getItemIds: function () {
            return root.song.artists.map(el => el.itemId);
        }
    }
    QA.CommentAction {
        itemId: root.itemId_
    }

    Action {
        enabled: root.song.source?.userId === QA.Global.session.user.userId
        icon.name: MD.Token.icon.delete
        text: qsTr('delete')
        onTriggered:
        // qr_tracks.playlistId = root.song.source?.itemId;
        {}
    }

    MD.Menu {
        title: qsTr('copy')
        QA.CopyAction {
            text: qsTr('title')
            icon.name: MD.Token.icon.title
            getCopyString: function () {
                return root.song.name;
            }
        }
        QA.CopyAction {
            text: qsTr('album')
            icon.name: MD.Token.icon.album
            getCopyString: function () {
                return root.song.album.name;
            }
        }
        QA.CopyAction {
            text: qsTr('url')
            icon.name: MD.Token.icon.link
            getCopyString: function () {
                return QA.Global.server_url(root.song.itemId);
            }
        }
    }

    QA.SongDetailQuery {
        id: qr_detail
        itemId: root.itemId
    }

    //QNcm.PlaylistTracksQuerier {
    //    id: qr_tracks
    //    operation: QNcm.PlaylistTracksQuerier.Del
    //    trackIds: [root.song.itemId]
    //    autoReload: playlistId.valid()
    //    onStatusChanged: {
    //        if (status === QA.enums.Finished) {
    //            QA.App.playqueueChanged();
    //        }
    //    }
    //}
}
