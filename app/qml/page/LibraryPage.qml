pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root

    readonly property bool canBack: false//leaf.folded && leaf.rightAbove
    title: m_content.currentItem?.title ?? qsTr("library")
    property int vpadding: showHeader ? 0 : MD.MProp.size.verticalPadding
    scrolling: m_content.currentItem?.scrolling ?? false

    property int currentIndex: 0

    function back() {
        m_content.pop(null);
    }

    readonly property Item libView: Item {
        visible: false
        implicitHeight: m_stack_layout.implicitHeight
        implicitWidth: m_stack_layout.implicitWidth
        clip: true

        MD.FlickablePane {
            corners: MD.Util.corners(0, MD.Token.shape.corner.large)
            view: {
                const item = m_stack_layout.itemAt(m_stack_layout.currentIndex);
                if (!item)
                    return null;
                const displayMode = ((item as ListView).headerItem as HeaderToolBar)?.displayMode ?? 0;
                return displayMode == 0 ? null : (item as ListView);
            }
        }

        MD.WidthProvider {
            id: m_wp
            total: m_stack_layout.width
            minimum: 140
            spacing: 12
            leftMargin: 8
            rightMargin: 8
        }

        StackLayout {
            id: m_stack_layout
            anchors.fill: parent
            Component.onCompleted: {
                currentIndex = Qt.binding(function () {
                    return root.currentIndex;
                });
            }

            BaseView {
                id: view_albumlist
                busy: qr_albums.querying
                delegate: {
                    const d = displayMode;
                    return [dg_albumlist, dg_album_card, dg_album_card][d];
                }
                model: qr_albums.data
                type: 'album'
                header: HeaderToolBar {
                    model: m_album_sort_type
                }
            }

            BaseView {
                id: m_view_mix
                busy: qr_mix.querying
                delegate: dg_playlist
                model: qr_mix.data
            }
            BaseView {
                id: m_view_album_artists
                busy: qr_album_artists.querying
                model: qr_album_artists.data
                type: 'artist'
                delegate: {
                    const d = displayMode;
                    return [dg_artistlist, dg_artist_card, dg_artist_card][d];
                }

                header: HeaderToolBar {
                    model: m_album_artist_sort_type
                }
            }

            BaseView {
                id: view_artistlist
                busy: qr_artists.querying
                model: qr_artists.data
                type: 'artist'
                delegate: {
                    const d = displayMode;
                    return [dg_artistlist, dg_artist_card, dg_artist_card][d];
                }

                header: HeaderToolBar {
                    model: m_artist_sort_type
                }
            }
        }
    }

    readonly property list<string> tabs: [qsTr("Album"), qsTr("Mix"), qsTr("AlbumArtist"), qsTr("Artist"),]
    readonly property Item chipBar: Item {
        visible: false
        implicitHeight: children[0].implicitHeight + 16
        MD.HorizontalListView {
            anchors.centerIn: parent
            implicitHeight: contentItem.childrenRect.height
            spacing: 12
            leftMargin: 8
            rightMargin: 8
            width: Math.min(implicitWidth, parent.width)
            model: root.tabs

            delegate: MD.FilterChip {
                required property string modelData
                required property int index
                checkable: false
                checked: index == root.currentIndex
                text: modelData
                onClicked: root.currentIndex = index
            }
        }
    }

    readonly property MD.TabBar tabBar: MD.TabBar {
        visible: false
        corners: MD.Util.corners(root.radius, 0)
        Repeater {
            model: root.tabs
            MD.TabButton {
                required property string modelData
                text: modelData
            }
        }
    }

    readonly property Item pageView: QA.PageContainer {
        id: m_content
        visible: false
        property var currentItemId: null
        MD.MProp.page: m_page_context
        MD.PageContext {
            id: m_page_context
            showHeader: false
            radius: root.radius
        }
        function route(itemId) {
            if (visible) {
                currentItemId = itemId;
                let url = itemId.pageUrl;
                switchTo(url, {
                    "itemId": itemId
                }, false);
            } else {
                QA.Action.route_by_id(itemId);
            }
        }
        initialItem: Item {}
    }

    QA.AlbumSortTypeModel {
        id: m_album_sort_type
    }
    QA.ArtistSortTypeModel {
        id: m_artist_sort_type
    }
    QA.ArtistSortTypeModel {
        id: m_album_artist_sort_type
    }

    QA.AlbumsQuery {
        id: qr_albums
        asc: m_album_sort_type.asc
        sort: m_album_sort_type.currentType
        onAscChanged: reload()
        onSortChanged: reload()
        Component.onCompleted: reload()
    }
    QA.ArtistsQuery {
        id: qr_artists
        asc: m_artist_sort_type.asc
        sort: m_artist_sort_type.currentType
        onAscChanged: reload()
        onSortChanged: reload()
        Component.onCompleted: reload()
    }
    QA.AlbumArtistsQuery {
        id: qr_album_artists
        asc: m_album_artist_sort_type.asc
        sort: m_album_artist_sort_type.currentType
        Component.onCompleted: reload()
    }
    QA.MixesQuery {
        id: qr_mix
        Component.onCompleted: reload()
    }

    Component {
        id: dg_albumlist
        BaseItem {
            image: QA.Util.image_url(model.itemId)
            text: model.name
            supportText: {
                const ex = QA.Store.extra(model.itemId);
                const tc = model.trackCount;
                const trackInfo = tc > 0 ? qsTr(`${tc} tracks`) : qsTr('no track');
                return [QA.Util.joinName(ex?.artists, '/'), trackInfo].filter(e => !!e).join(' - ');
            }
        }
    }
    Component {
        id: dg_album_card
        QA.AlbumCardDelegate {
            widthProvider: m_wp
            mdState.backgroundOpacity: (ListView.view as BaseView).displayMode == QA.Enum.DGrid ? 0 : 1
            onClicked: {
                m_content.route(model.itemId);
                ListView.view.currentIndex = index;
            }
        }
    }
    Component {
        id: dg_artist_card
        QA.ArtistCardDelegate {
            widthProvider: m_wp
            mdState.backgroundOpacity: (ListView.view as BaseView).displayMode == QA.Enum.DGrid ? 0 : 1
            onClicked: {
                m_content.route(model.itemId);
                ListView.view.currentIndex = index;
            }
        }
    }
    Component {
        id: dg_artistlist
        BaseItem {
            image: QA.Util.image_url(model.itemId)
            text: model.name
            // supportText: `${model.albumCount} albums`
        }
    }
    Component {
        id: dg_playlist
        BaseItem {
            image: QA.Util.image_url(model.itemId)
            text: model.name
            supportText: `${model.trackCount} songs`
        }
    }
    Component {
        id: dg_djradiolist
        BaseItem {
            image: QA.Util.image_url(model.itemId)
            text: model.name
            supportText: `${model.programCount} programs`
        }
    }

    MD.SplitView {
        anchors.fill: parent
        ColumnLayout {
            LayoutItemProxy {
                Layout.fillWidth: true
                target: root.chipBar
            }

            LayoutItemProxy {
                Layout.fillHeight: true
                Layout.fillWidth: true
                target: root.libView
            }
        }
    }

    // QA.Leaflet {
    //     id: leaf
    //     anchors.fill: parent
    //     rightAbove: m_content.depth === 2
    //     leftMin: 280
    //     rightMin: 400

    //     leftPage: MD.Pane {
    //         topPadding: root.vpadding
    //         showBackground: false

    //         ColumnLayout {
    //             id: p1
    //             anchors.fill: parent
    //             spacing: 0
    //         }
    //     }
    //     rightPage: MD.StackView {}
    // }

    component BaseView: MD.VerticalListView {
        id: m_view_base
        bottomMargin: root.vpadding

        property bool dirty: false
        property string type
        property int displayMode: (headerItem as HeaderToolBar)?.displayMode ?? 0
        clip: false

        currentIndex: -1
        highlightMoveDuration: 1000
        highlightMoveVelocity: -1
        cacheBuffer: 300
        displayMarginBeginning: 300
        displayMarginEnd: 300

        footer: ColumnLayout {
            width: parent.width
            MD.Space {
                spacing: 8
            }
            QA.SyncingLabel {
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: parent.width
            }
            MD.Space {
                spacing: 8
            }
        }
    }

    component BaseItem: MD.ListItem {
        id: m_r
        property var itemId: model.itemId
        property string image

        width: ListView.view.width
        maximumLineCount: 2

        corners: MD.Util.corners(0, index + 1 == count ? root.radius : 0)

        leader: QA.Image {
            radius: 8
            source: m_r.image
            implicitWidth: displaySize.width
            implicitHeight: displaySize.height

            displaySize: Qt.size(48, 48)
        }
        rightPadding: 0
        trailing: MD.IconButton {
            MD.MProp.textColor: MD.Token.color.on_surface_variant
            icon.name: MD.Token.icon.more_vert
            onClicked: {
                QA.Action.menu_by_id(m_r.itemId, this);
            }
        }
        onClicked: {
            m_content.route(itemId);
            ListView.view.currentIndex = index;
        }
    }

    component HeaderToolBar: MD.Control {
        id: m_header_bar
        width: ListView.view.width
        horizontalPadding: 8
        property var model
        property int displayMode: 0

        QA.SortMenu {
            id: m_header_sort_menu
            y: m_header_bar.height
            model: m_header_bar.model
        }

        contentItem: RowLayout {
            QA.OrderChip {
                Layout.alignment: Qt.AlignVCenter
                text: {
                    const m = m_header_bar.model;
                    m.item(m.currentIndex).name;
                }
                asc: m_header_bar.model.asc
                onClicked: {
                    m_header_sort_menu.open();
                }
            }
            Item {
                Layout.fillWidth: true
            }
            Row {
                Layout.alignment: Qt.AlignVCenter
                MD.StandardIconButton {
                    anchors.verticalCenter: parent.verticalCenter
                    icon.name: QA.Util.displayModeIcon(m_header_bar.displayMode)
                    icon.width: 22
                    icon.height: 22
                    implicitBackgroundSize: 0
                    onClicked: {
                        const popup = MD.Util.showPopup(m_display_mode_menu, {
                            displayMode: m_header_bar.displayMode
                        }, this);
                    }
                    Component {
                        id: m_display_mode_menu
                        QA.DisplayModeMenu {
                            y: parent.height
                            modal: true
                            dim: false
                            onDisplayModeChanged: {
                                m_header_bar.displayMode = displayMode;
                            }
                        }
                    }
                }
                MD.StandardIconButton {

                    anchors.verticalCenter: parent.verticalCenter
                    icon.name: MD.Token.icon.filter_list
                    icon.width: 22
                    icon.height: 22
                    implicitBackgroundSize: 0
                }
            }
        }
        background: Rectangle {
            color: MD.MProp.color.surface
        }
    }
}
