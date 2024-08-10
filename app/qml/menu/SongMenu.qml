import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Menu {
    id: root

    required property QA.t_song song

    dim: false
    font.capitalization: Font.Capitalize
    modal: true

    QA.PlaynextAction {
        enabled: root.song.itemId !== QA.Global.cur_song.itemId
        song: root.song
    }
    Action {
        icon.name: MD.Token.icon.queue
        text: qsTr('Add to Playlist')
        onTriggered: {
            QA.Global.show_page_popup('qrc:/Qcm/App/qml/page/FavPage.qml', {
                songId: root.song.itemId
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
    Action {
        icon.name: MD.Token.icon.person
        text: qsTr('go to artist')

        onTriggered: {
            const artists = root.song.artists;
            if (artists.length === 1)
                QA.Global.route(artists[0].itemId);
            else
                QA.Global.show_page_popup('qrc:/Qcm/App/qml/component/ArtistsPopup.qml', {
                    "model": artists
                });
        }
    }
    QA.CommentAction {
        itemId: root.song.itemId
    }

    Action {
        enabled: root.song.source?.userId === QA.Global.user_info.userId
        icon.name: MD.Token.icon.delete
        text: qsTr('delete')
        onTriggered: {
            qr_tracks.playlistId = root.song.source?.itemId;
        }
    }

    MD.Menu {
        title: qsTr('copy')
        Action {
            text: qsTr('title')
            icon.name: MD.Token.icon.title
            onTriggered: {
                QA.Clipboard.text = root.song.name;
                QA.Global.toast(qsTr("Copied to clipboard"));
            }
        }
        Action {
            text: qsTr('url')
            icon.name: MD.Token.icon.link
            onTriggered: {
                QA.Clipboard.text = QA.Global.server_url(root.song.itemId);
                QA.Global.toast(qsTr("Copied to clipboard"));
            }
        }
    }

    QNcm.PlaylistTracksQuerier {
        id: qr_tracks
        operation: QNcm.PlaylistTracksQuerier.Del
        trackIds: [root.song.itemId]
        autoReload: playlistId.valid()
        onStatusChanged: {
            if (status === QA.enums.Finished) {
                QA.App.playlistChanged();
            }
        }
    }
}
