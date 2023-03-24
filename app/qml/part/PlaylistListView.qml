import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QcmApp
import ".."
import "../component"

Pane {
    property alias cat: qr_pl.cat

    verticalPadding: 0

    GridView {
        property int cellWidth_: 180

        anchors.fill: parent
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        model: qr_pl.data
        cellHeight: 250
        cellWidth: width > 0 ? width / Math.floor((width / cellWidth_)) : 0

        delegate: PicGridDelegate {
            text: model.name
            // subText:
            image.source: `image://ncm/${model.picUrl}`
            onClicked: {
                QA.route(model.itemId);
            }
        }

        footer: ListBusyFooter {
            width: GridView.view.width
            running: qr_pl.status === ApiQuerierBase.Querying
        }

        ScrollBar.vertical: ScrollBar {
        }

    }

    ApiContainer {
        PlaylistListQuerier {
            id: qr_pl

            autoReload: cat.length > 0
        }

    }

}
