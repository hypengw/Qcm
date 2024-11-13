import QtQuick
import Qcm.App as QA
import QtQuick.Templates as T
import Qcm.Material as MD
import Qcm.Service.Ncm as QNcm

MD.Control {
    id: root
    padding: 0
    property alias cat: qr_pl.cat
    property MD.t_corner corners

    background: Item {
        implicitWidth: 0
        implicitHeight: 0
        MD.FlickablePane {
            view: m_view
            contentWidth: view.contentItem.width
            contentHeight: view.contentItem.height
            corners: MD.Util.corner(0, root.MD.MatProp.page.radius)
            color: root.MD.MatProp.backgroundColor
        }
    }

    contentItem: QA.GridView {
        id: m_view
        model: qr_pl.data
        fixedCellWidth: QA.Util.dynCardWidth(widthNoMargin, spacing)

        delegate: QA.PicCardGridDelegate {
            text: model.name
            // subText:
            image.source: QA.Util.image_url(model.picUrl)
            onClicked: {
                QA.Global.route(model.itemId);
            }
        }

        footer: MD.ListBusyFooter {
            width: GridView.view.width
            running: qr_pl.status === QA.enums.Querying
        }

        QNcm.PlaylistListQuerier {
            id: qr_pl

            autoReload: cat.length > 0
        }
    }
}
