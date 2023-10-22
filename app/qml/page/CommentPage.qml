import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    padding: 0
    title: `Comment(${view.model.total})`

    property var itemId

    MD.ListView {
        id: view
        anchors.fill: parent
        clip: true
        implicitHeight: contentHeight
        busy: model_loader.item.status === QA.ApiQuerierBase.Querying
        model: model_loader.item.data

        delegate: QA.CommentDelegate {
        }
        Loader {
            id: model_loader
            sourceComponent: cm_album
        }

        Component {
            id: cm_album
            QA.CommentAlbumQuerier {
                itemId: root.itemId
                autoReload: itemId.valid()
            }
        }
    }
}
