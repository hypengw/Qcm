import QtQuick
import Qcm.App as QA
import Qcm.Material as MD
import Qcm.Msg as QM

MD.Menu {
    id: root

    required property QA.item_id itemId
    property QA.item_id sourceId
    property bool canDelete: false
    // no aot, it's bugly
    property var song: QA.App.empty.song
    readonly property QA.EntityView entity: QA.Store.item(root.itemId)
    readonly property list<var> artists: entity?.artists ?? []

    dim: false
    font.capitalization: Font.Capitalize
    modal: true

    delegate: MD.ActionMenuItem {
        font.capitalization: root.font.capitalization
    }

    QA.PlayAction {
        itemId: root.itemId
    }

    QA.PlayNextAction {
        itemId: root.itemId
    }

    QA.AddToMixAction {
        songIds: [root.itemId]
    }

    QA.GoToAlbumAction {
        albumId: root.entity?.albumId ?? QA.Util.createItemid()
    }

    QA.GoToArtistAction {
        enabled: root.artists.length > 0
        getItemIds: function () {
            return root.artists.map(el => QA.Util.artistId(el.id_proto));
        }
    }
    QA.CommentAction {
        itemId: root.itemId
    }

    MD.Action {
        enabled: root.canDelete
        icon.name: MD.Token.icon.delete
        text: qsTr('delete')
        onTriggered:
        //m_qr_manipulate.mixId = root.sourceId;
        //m_qr_manipulate.itemIds = [root.itemId];
        //m_qr_manipulate.reload();
        {}
    }

    MD.Menu {
        title: qsTr('copy')
        delegate: root.delegate
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
                return root.entity?.album?.name ?? "";
            }
        }
        QA.CopyAction {
            text: qsTr('url')
            icon.name: MD.Token.icon.link
            getCopyString: function () {
                return '';//QA.Global.server_url(root.itemId);
            }
        }
    }
    // QA.MixManipulateQuery {
    //     id: m_qr_manipulate
    //     oper: QA.Enum.ManipulateMixDel
    // }
}
