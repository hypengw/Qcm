pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD
import Qcm.Msg as QM

MD.Page {
    id: root

    component Block: Column {
        id: m_block
        property string title
        property alias model: m_view.model
        visible: m_view.count > 0
        MD.Label {
            text: m_block.title
            typescale: MD.Token.typescale.headline_medium
        }
        MD.HorizontalListView {
            id: m_view
            height: 400
            width: parent.width
            spacing: 12
            delegate: QA.PicCardDelegate {
                required property var model
                text: model.name
                image.source: QA.Util.image_url(model.itemId)
                width: m_width_provider.width
                picWidth: width
            }

            MD.WidthProvider {
                id: m_width_provider
                minimum: 160
                spacing: m_view.spacing
                total: m_view.width
            }
        }
    }

    MD.VerticalFlickable {
        anchors.fill: parent
        Column {
            anchors.fill: parent
            Block {
                width: parent.width
                title: "Recommands"
            }
            Block {
                width: parent.width
                title: "Recently Added"
                model: m_recent_query.data
                AlbumsQuery {
                    id: m_recent_query
                    sort: QM.AlbumSort.ALBUM_SORT_ADDED_TIME
                    Component.onCompleted: reload()
                }
            }
            Block {
                width: parent.width
                title: "Last Played"
            }
        }
    }
}
