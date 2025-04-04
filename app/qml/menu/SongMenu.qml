import QtQuick
import QtQuick.Controls.Basic
import Qcm.App as QA
import Qcm.Material as MD

MD.Menu {
    id: root

    property QA.t_id itemId
    property QA.t_id sourceId
    property bool canDelete: false
    property var song//: qr_detail.data
    readonly property QA.t_id itemId_: itemId.valid() ? itemId : song.itemId

    dim: false
    font.capitalization: Font.Capitalize
    modal: true

    QA.PlaynextAction {
        enabled: root.itemId_ !== QA.App.playqueue.currentSong.itemId
        songId: root.itemId_
    }

    QA.AddToMixAction {
        songId: root.itemId_
    }

    QA.GoToAlbumAction {
        albumId: root.song.album.itemId
    }

    QA.GoToArtistAction {
        enabled: root.song.artists.length > 1 || root.song.artists[0]?.itemId.valid()
        getItemIds: function () {
            return root.song.artists.map(el => el.itemId);
        }
    }
    QA.CommentAction {
        itemId: root.itemId_
    }

    Action {
        enabled: root.canDelete
        icon.name: MD.Token.icon.delete
        text: qsTr('delete')
        onTriggered: {
             //m_qr_manipulate.mixId = root.sourceId;
             //m_qr_manipulate.itemIds = [root.itemId_];
             //m_qr_manipulate.reload();
        }
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

    // QA.SongDetailQuery {
    //     id: qr_detail
    //     itemId: root.itemId
    // }

    // QA.MixManipulateQuery {
    //     id: m_qr_manipulate
    //     oper: QA.Enum.ManipulateMixDel
    // }
}
