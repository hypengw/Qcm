import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Page {
    id: root
    padding: 0
    title: `Comment(${view.model.total})`

    property var itemId

    MD.ListView {
        id: view
        anchors.fill: parent
        implicitHeight: contentHeight
        busy: model_loader.item.status === QA.enums.Querying
        model: model_loader.item.data

        delegate: QA.CommentDelegate {
        }
        Loader {
            id: model_loader
            sourceComponent: cm_querier
        }

        Component {
            id: cm_querier
            QNcm.CommentsQuerier {
                itemId: root.itemId
                autoReload: itemId.valid()
            }
        }
    }
}
