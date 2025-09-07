pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD
import Qcm.Msg as QM

MD.Page {
    id: root
    horizontalPadding: 0
    onVisibleChanged: m_last_played_timer.start()

    component Block: Column {
        id: m_block
        property string title
        property alias model: m_view.model
        property alias delegate: m_view.delegate
        property alias widthProvider: m_width_provider
        property alias view: m_view
        visible: m_view.count > 0
        spacing: 8
        QA.HorizontalItemBar {
            width: parent.width
            title: m_block.title
            view: m_view
        }
        MD.HorizontalListView {
            id: m_view
            height: 100 + m_width_provider.width
            width: parent.width
            spacing: 12
            highlightRangeMode: ListView.StrictlyEnforceRange

            MD.WidthProvider {
                id: m_width_provider
                minimum: 140
                spacing: m_view.spacing
                total: m_view.width
            }
        }
    }
    component AlbumCard: QA.ImagePlayCard {
        required property var model
        itemId: model.itemId
        text: model.name
        image.source: QA.Util.image_url(model.itemId)
        subText: QA.Util.joinName(model.extra?.artists)
        picWidth: width

        onClicked: {
            QA.Action.routeItem(model.itemId);
        }
    }

    MD.VerticalFlickable {
        anchors.fill: parent
        rightMargin: 0
        Column {
            spacing: 0
            width: parent.width
            Block {
                width: parent.width
                title: "Recommands"
            }
            Block {
                id: m_recent_block
                width: parent.width
                title: "Recently Added"
                model: m_recent_query.data
                delegate: AlbumCard {
                    width: m_recent_block.widthProvider.width
                }
                QA.AlbumsQuery {
                    id: m_recent_query
                    sort: QM.AlbumSort.ALBUM_SORT_ADDED_TIME
                    asc: false
                    limit: 20
                    noMore: true
                    Component.onCompleted: reload()
                }
            }
            Block {
                id: m_last_played_block
                width: parent.width
                title: "Last Played"
                model: m_last_played_query.data
                delegate: AlbumCard {
                    width: m_recent_block.widthProvider.width
                }
                QA.AlbumsQuery {
                    id: m_last_played_query
                    sort: QM.AlbumSort.ALBUM_SORT_LAST_PLAYED_AT
                    asc: false
                    property QM.albumFilter filter1
                    filter1.lastPlayedAtFilter: {
                        const f = QA.Util.lastPlayedAtFilter();
                        f.condition = QM.DateCondition.DATE_CONDITION_NOT_NULL;
                        return f;
                    }
                    filters: [filter1]
                    limit: 20
                    noMore: true
                    Component.onCompleted: reload()
                }
                Timer {
                    id: m_last_played_timer
                    interval: 500
                    onTriggered: {
                        if (root.visible) {
                            const view = m_last_played_block.view;
                            view.currentIndex = -1;
                            view.contentX = 0;
                            m_last_played_query.reload();
                        }
                    }
                }

                Connections {
                    target: QA.Action
                    function onPlayLog() {
                        m_last_played_timer.start();
                    }
                }
            }
        }
    }
}
