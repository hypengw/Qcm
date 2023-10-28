import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Pane {
    property alias cat: qr_pl.cat

    padding: 0

    QA.MGridView {
        anchors.fill: parent
        clip: true
        model: qr_pl.data

        delegate: Item {
            width: GridView.view.cellWidth
            height: GridView.view.cellHeight
            QA.PicGridDelegate {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 8

                width: picWidth
                height: Math.min(implicitHeight, parent.height)
                text: model.name
                // subText:
                image.source: `image://ncm/${model.picUrl}`
                onClicked: {
                    QA.Global.route(model.itemId);
                }
            }
        }

        footer: MD.ListBusyFooter {
            width: GridView.view.width
            running: qr_pl.status === QA.ApiQuerierBase.Querying
        }
    }

    QA.PlaylistListQuerier {
        id: qr_pl

        autoReload: cat.length > 0
    }
}
