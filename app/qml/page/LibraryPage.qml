pragma ComponentBehavior: Bound
import QtCore
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
            corners: MD.Util.corners(MD.Token.shape.corner.medium, MD.Token.shape.corner.large)
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
                id: m_view_album
                busy: qr_albums.querying
                displayMode: m_album_setting.display_mode
                delegate: {
                    const d = displayMode;
                    return [dg_albumlist, dg_album_card, dg_album_card][d];
                }
                model: qr_albums.data
                type: 'album'
                header: HeaderToolBar {
                    model: m_album_sort_type
                    displayMode: m_view_album.displayMode
                    onSelectDisplayMode: m => m_album_setting.display_mode = m
                    filterModel: m_album_filter_model
                }
            }

            BaseView {
                id: m_view_mix
                busy: qr_mix.querying
                delegate: dg_playlist
                model: qr_mix.data

                header: HeaderToolBar {
                    model: m_mix_sort_type
                    displayMode: m_view_mix.displayMode
                    onSelectDisplayMode: m => m_mix_setting.display_mode = m
                    filterModel: m_mix_filter_model
                }
            }
            BaseView {
                id: m_view_album_artist
                busy: qr_album_artists.querying
                displayMode: m_album_artist_setting.display_mode
                model: qr_album_artists.data
                type: 'albumartist'
                delegate: {
                    const d = displayMode;
                    return [dg_artistlist, dg_artist_card, dg_artist_card][d];
                }

                header: HeaderToolBar {
                    model: m_album_artist_sort_type
                    displayMode: m_view_album_artist.displayMode
                    onSelectDisplayMode: m => m_album_artist_setting.display_mode = m
                    filterModel: m_album_artist_filter_model
                }
            }

            BaseView {
                id: m_view_artist
                busy: qr_artists.querying
                model: qr_artists.data
                displayMode: m_artist_setting.display_mode
                type: 'artist'
                delegate: {
                    const d = displayMode;
                    return [dg_artistlist, dg_artist_card, dg_artist_card][d];
                }

                header: HeaderToolBar {
                    model: m_artist_sort_type
                    displayMode: m_view_artist.displayMode
                    onSelectDisplayMode: m => m_artist_setting.display_mode = m
                    filterModel: m_artist_filter_model
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
                QA.Action.routeItem(itemId);
            }
        }
        initialItem: Item {}
    }

    Component {
        id: dg_albumlist
        BaseItem {
            image: QA.Util.image_url(model.itemId)
            text: model.name
            supportText: {
                const ex = model.extra;
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
                const itemId = model.itemId.clone();
                const view = ListView.view as BaseView;
                if (view?.type == "artist") {
                    itemId.type = QA.Enum.ItemArtist;
                }
                m_content.route(itemId);
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

    component BaseView: QA.ItemView {
        id: m_view_base
        bottomMargin: root.vpadding

        property bool dirty: false
        property string type
        clip: false

        currentIndex: -1
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

    component BaseItem: QA.ListItemDelegate {
        onClicked: {
            const view = ListView.view as BaseView;
            if (view?.type == "artist") {
                const itemId_ = itemId.clone();
                itemId_.type = QA.Enum.ItemArtist;
                m_content.route(itemId_);
            } else {
                m_content.route(itemId);
            }
        }
    }

    component HeaderToolBar: MD.Control {
        id: m_header_bar
        width: ListView.view.width
        horizontalPadding: 8
        property var model
        property int displayMode: 0
        property QA.FilterRuleModel filterModel
        signal selectDisplayMode(int mode)

        QA.SortMenu {
            id: m_header_sort_menu
            y: m_header_bar.height
            model: m_header_bar.model
        }

        verticalPadding: 4

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
                MD.SmallIconButton {
                    id: m_display_mode_btn
                    anchors.verticalCenter: parent.verticalCenter
                    action: QA.SelectDisplayModeAction {
                        menuParent: m_display_mode_btn
                        displayMode: m_header_bar.displayMode
                        onSelectDisplayMode: m => m_header_bar.selectDisplayMode(m)
                    }
                }
                MD.SmallIconButton {
                    anchors.verticalCenter: parent.verticalCenter
                    action: QA.FilterAction {
                        model: m_header_bar.filterModel
                    }
                }
            }
        }
        background: Rectangle {
            topLeftRadius: MD.Token.shape.corner.medium
            topRightRadius: MD.Token.shape.corner.medium
            color: MD.MProp.color.surface
        }
    }

    QA.AlbumSortTypeModel {
        id: m_album_sort_type
    }
    QA.MixSortTypeModel {
        id: m_mix_sort_type
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
        onAscChanged: delayReload()
        onSortChanged: delayReload()
        onFiltersChanged: delayReload()
        Component.onCompleted: delayReload()
    }
    QA.ArtistsQuery {
        id: qr_artists
        asc: m_artist_sort_type.asc
        sort: m_artist_sort_type.currentType
        onAscChanged: delayReload()
        onSortChanged: delayReload()
        onFiltersChanged: delayReload()
        Component.onCompleted: delayReload()
    }
    QA.AlbumArtistsQuery {
        id: qr_album_artists
        asc: m_album_artist_sort_type.asc
        sort: m_album_artist_sort_type.currentType
        onAscChanged: delayReload()
        onSortChanged: delayReload()
        onFiltersChanged: delayReload()
        Component.onCompleted: delayReload()
    }
    QA.MixesQuery {
        id: qr_mix
        asc: m_mix_sort_type.asc
        sort: m_mix_sort_type.currentType
        onAscChanged: delayReload()
        onSortChanged: delayReload()
        onFiltersChanged: delayReload()
        Component.onCompleted: reload()
    }

    QA.ArtistFilterRuleModel {
        id: m_album_artist_filter_model
        function doQuery() {
            const q = qr_album_artists;
            q.filters = this.items();
        }
        onApply: {
            doQuery();
            m_album_artist_setting.filter = toJson();
            m_album_artist_setting.sync();
        }
        onReset: {
            fromJson(m_album_artist_setting.filter);
        }
    }

    QA.ArtistFilterRuleModel {
        id: m_artist_filter_model
        function doQuery() {
            const q = qr_artists;
            q.filters = this.items();
        }
        onApply: {
            doQuery();
            m_artist_setting.filter = toJson();
            m_artist_setting.sync();
        }
        onReset: {
            fromJson(m_artist_setting.filter);
        }
    }

    QA.AlbumFilterRuleModel {
        id: m_album_filter_model
        function doQuery() {
            const q = qr_albums;
            q.filters = this.items();
        }
        onApply: {
            doQuery();
            m_album_setting.filter = toJson();
            m_album_setting.sync();
        }
        onReset: {
            fromJson(m_album_setting.filter);
        }
    }

    QA.AlbumFilterRuleModel {
        id: m_mix_filter_model
        function doQuery() {
            const q = qr_mix;
            q.filters = this.items();
        }
        onApply: {
            doQuery();
            m_mix_setting.filter = toJson();
            m_mix_setting.sync();
        }
        onReset: {
            fromJson(m_mix_setting.filter);
        }
    }

    Settings {
        category: "library"
        property alias index: root.currentIndex
    }

    Settings {
        id: m_album_setting
        category: "library.album"
        property int display_mode: 0
        property alias sort: m_album_sort_type.currentType
        property alias asc: m_album_sort_type.asc
        property string filter
        Component.onCompleted: {
            m_album_filter_model.reset();
            m_album_filter_model.doQuery();
        }
    }

   Settings {
        id: m_mix_setting
        category: "library.mix"
        property int display_mode: 0
        property alias sort: m_mix_sort_type.currentType
        property alias asc: m_mix_sort_type.asc
        property string filter
        Component.onCompleted: {
            m_mix_filter_model.reset();
            m_mix_filter_model.doQuery();
        }
    }

    Settings {
        id: m_artist_setting
        category: "library.artist"
        property int display_mode: 0
        property alias sort: m_artist_sort_type.currentType
        property alias asc: m_artist_sort_type.asc
        property string filter
        Component.onCompleted: {
            m_artist_filter_model.reset();
            m_artist_filter_model.doQuery();
        }
    }

    Settings {
        id: m_album_artist_setting
        category: "library.album_artist"
        property int display_mode: 0
        property alias sort: m_album_artist_sort_type.currentType
        property alias asc: m_album_artist_sort_type.asc
        property string filter

        Component.onCompleted: {
            m_artist_filter_model.reset();
            m_artist_filter_model.doQuery();
        }
    }
}
