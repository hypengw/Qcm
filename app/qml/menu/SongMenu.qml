import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.Menu {
    id: root

    property QA.item_id itemId
    property QA.item_id sourceId
    property bool canDelete: false
    // no aot, it's bugly
    property var song: QA.App.empty.song
    readonly property QA.item_id _itemId: {
        return itemId.valid ? itemId : song.itemId;
    }
    readonly property list<var> artists: {
        const ex = QA.Store.extra(_itemId);
        return ex?.artists ?? [];
    }

    dim: false
    font.capitalization: Font.Capitalize
    modal: true

    QA.PlayAction {
        itemId: root._itemId
    }

    QA.PlaynextAction {
        itemId: root._itemId
    }

    QA.AddToMixAction {
        songId: root._itemId
    }

    QA.GoToAlbumAction {
        albumId: root.song.albumId
    }

    QA.GoToArtistAction {
        enabled: root.artists.length > 0
        getItemIds: function () {
            return root.artists.map(el => QA.Util.artistId(el.id));
        }
    }
    QA.CommentAction {
        itemId: root._itemId
    }

    MD.Action {
        enabled: root.canDelete
        icon.name: MD.Token.icon.delete
        text: qsTr('delete')
        onTriggered:
        //m_qr_manipulate.mixId = root.sourceId;
        //m_qr_manipulate.itemIds = [root._itemId];
        //m_qr_manipulate.reload();
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
    // QA.MixManipulateQuery {
    //     id: m_qr_manipulate
    //     oper: QA.Enum.ManipulateMixDel
    // }
}
