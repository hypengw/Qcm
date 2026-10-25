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
        property alias model: m_source.model
        property alias delegate: m_view.delegate
        property alias widthProvider: m_width_provider
        property alias view: m_view
        visible: m_view.count > 0
        spacing: 8
        QA.HorizontalItemBar {
            x: root.MD.MProp.page.leftMargin
            width: parent.width - root.MD.MProp.page.leftMargin - root.MD.MProp.page.rightMargin
            title: m_block.title
            view: m_view
        }
        QA.HomeItemSource {
            id: m_source
        }
        MD.PageHorizontalLazyList {
            id: m_view
            height: 100 + m_width_provider.width
            width: parent.width
            spacing: Math.max(12, rightMargin)
            source: m_source
            estimatedItemExtent: m_width_provider.width

            MD.WidthProvider {
                id: m_width_provider
                minimum: 140
                spacing: m_view.spacing
                total: m_view.width
                leftMargin: m_view.leftMargin
                rightMargin: m_view.rightMargin
            }
        }
    }

    component MImagePlayCard: Item {
        id: cardDelegate
        required property MD.LazyRow row
        readonly property var model: row.value
        property alias smallPlayButton: card.smallPlayButton
        property alias playButtonAlignment: card.playButtonAlignment
        property alias subText: card.subText

        QA.ImagePlayCard {
            id: card
            width: parent.width
            itemId: cardDelegate.model.itemId
            text: cardDelegate.model.name
            image.source: QA.Util.image_url(cardDelegate.model.itemId)
            picWidth: width
            onClicked: QA.Action.routeItem(cardDelegate.model.itemId)
        }
    }

    component AlbumCard: MImagePlayCard {
        smallPlayButton: true
        playButtonAlignment: Qt.AlignRight | Qt.AlignBottom
        subText: QA.Util.joinName(model.artists) || qsTr("%1 tracks").arg(model.trackCount)
    }
    component MixCard: MImagePlayCard {
        subText: qsTr("%1 songs").arg(model.trackCount)
    }
    QA.HomeRecommendationsQuery {
        id: m_home_query
        objectName: "homeRecommendations"
        Component.onCompleted: reload()
    }

    MD.Scrollable {
        anchors.fill: parent
        clip: true
        synchronousDrag: true
        flickableDirection: MD.Scrollable.VerticalFlick
        maximumFlickVelocity: MD.Token.flick.maximumFlickVelocity
        flickDeceleration: MD.Token.flick.flickDeceleration
        contentWidth: width
        contentHeight: m_content.height
        topMargin: MD.MProp.page?.topMargin ?? 0
        bottomMargin: MD.MProp.page?.bottomMargin ?? 0
        MD.NestedScroll.pressDelay: MD.Token.flick.pressDelay
        MD.NestedScroll.enabled: true
        MD.NestedScroll.axes: Qt.Vertical
        MD.ScrollBarBase.vertical: MD.ScrollBar {}

        Column {
            id: m_content
            spacing: 0
            width: parent.width
            Block {
                id: m_for_you_block
                objectName: "for_you"
                width: parent.width
                title: qsTr("For You")
                model: m_home_query.forYou
                delegate: Item {
                    id: forYouDelegate
                    required property MD.LazyRow row
                    width: m_for_you_block.widthProvider.width
                    QA.ForYouCard {
                        width: parent.width
                        model: forYouDelegate.row.value
                        query: m_home_query
                    }
                }
            }
            MD.Label {
                x: root.MD.MProp.page.leftMargin
                width: parent.width - root.MD.MProp.page.leftMargin - root.MD.MProp.page.rightMargin
                visible: m_home_query.error.length > 0
                text: m_home_query.error
                wrapMode: Text.Wrap
                color: MD.Token.color.error
            }
            Block {
                id: m_radio_block
                width: parent.width
                title: "Radio"
                model: m_home_query.radios
                delegate: Item {
                    id: radioDelegate
                    required property MD.LazyRow row
                    width: m_radio_block.widthProvider.width
                    QA.RadioQueueCard {
                        width: parent.width
                        model: radioDelegate.row.value
                        query: m_home_query
                    }
                }
            }
            Block {
                id: m_recommand_block
                width: parent.width
                title: "Recommand Mixes"
                model: m_recommand_query.data
                delegate: MixCard {
                    width: m_recommand_block.widthProvider.width
                }
                QA.RemoteMixesQuery {
                    id: m_recommand_query
                    asc: false
                    limit: 20
                    noMore: true
                    property QM.remoteMixFilter filter1
                    filter1.typeFilter: {
                        const f = QA.Util.typeStringFilter();
                        f.value = "recommand";
                        f.condition = QM.TypeCondition.TYPE_CONDITION_IS;
                        return f;
                    }
                    filters: [filter1]
                }
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
                    width: m_last_played_block.widthProvider.width
                }
                QA.AlbumsQuery {
                    id: m_last_played_query
                    property bool resetPositionAfterReload: false
                    onStatusChanged: {
                        if (status === QA.Enum.Finished && resetPositionAfterReload) {
                            resetPositionAfterReload = false;
                            m_last_played_block.view.positionAtIndex(0);
                        }
                    }
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
                            m_last_played_query.resetPositionAfterReload = true;
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
