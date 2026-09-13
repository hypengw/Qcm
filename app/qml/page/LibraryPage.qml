pragma ComponentBehavior: Bound
import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qcm.App as QA
import Qcm.Material as MD
import Qcm.Msg as QM

MD.Page {
    id: root
    rightPadding: MD.MProp.size.isCompact ? 0 : 8
    topPadding: vpadding

    readonly property bool canBack: false//leaf.folded && leaf.rightAbove
    title: m_content.currentItem?.title ?? qsTr("library")
    property int vpadding: showHeader ? 0 : MD.MProp.size.verticalPadding
    scrolling: m_content.currentItem?.scrolling ?? false

    property int currentIndex: 0

    function back() {
        m_content.pop(null);
    }

    readonly property Item libView: Item {
        id: m_lib_view

        visible: false
        implicitHeight: m_library_layout.implicitHeight
        implicitWidth: m_library_layout.implicitWidth
        clip: true

        readonly property BaseView currentView: m_stack_layout.itemAt(m_stack_layout.currentIndex) as BaseView

        ColumnLayout {
            id: m_library_layout
            anchors.fill: parent
            spacing: 0

            HeaderToolBar {
                Layout.fillWidth: true
                view: m_lib_view.currentView
            }

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
                clip: true

                MD.FlickablePane {
                    corners: MD.Util.corners(0, MD.Token.shape.corner.large)
                    view: {
                        const view = m_lib_view.currentView;
                        return view?.displayMode == 0 ? null : view;
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
                        filterModel: m_album_filter_model
                        headerActions: [
                            QA.PlayAllAction {
                                albumSort: qr_albums.sort
                                albumAsc: qr_albums.asc
                                filters: qr_albums.filters
                            },
                            QA.PlayAllAction {
                                icon.name: MD.Token.icon.shuffle
                                text: qsTr('shuffle all')
                                albumSort: QM.AlbumSort.ALBUM_SORT_RANDOM
                                albumAsc: qr_albums.asc
                                filters: qr_albums.filters
                            }
                        ]
                        model: qr_albums.data
                        sortModel: m_album_sort_type
                        type: 'album'
                        onSelectDisplayMode: m => m_album_setting.display_mode = m
                    }

                    BaseView {
                        id: m_view_mix
                        busy: qr_mix.querying
                        displayMode: m_mix_setting.display_mode
                        filterModel: m_mix_filter_model
                        headerActions: [
                            QA.MixCreateAction {},
                            QA.MixLinkAction {}
                        ]
                        model: qr_mix.data
                        sortModel: m_mix_sort_type
                        onSelectDisplayMode: m => m_mix_setting.display_mode = m

                        delegate: {
                            const d = displayMode;
                            return [dg_mixlist, dg_mix_card, dg_mix_card][d];
                        }
                    }
                    BaseView {
                        id: m_view_album_artist
                        busy: qr_album_artists.querying
                        displayMode: m_album_artist_setting.display_mode
                        filterModel: m_album_artist_filter_model
                        headerActions: []
                        model: qr_album_artists.data
                        sortModel: m_album_artist_sort_type
                        type: 'albumartist'
                        onSelectDisplayMode: m => m_album_artist_setting.display_mode = m
                        delegate: {
                            const d = displayMode;
                            return [dg_artistlist, dg_artist_card, dg_artist_card][d];
                        }
                    }

                    BaseView {
                        id: m_view_artist
                        busy: qr_artists.querying
                        displayMode: m_artist_setting.display_mode
                        filterModel: m_artist_filter_model
                        headerActions: []
                        model: qr_artists.data
                        sortModel: m_artist_sort_type
                        type: 'artist'
                        onSelectDisplayMode: m => m_artist_setting.display_mode = m
                        delegate: {
                            const d = displayMode;
                            return [dg_artistlist, dg_artist_card, dg_artist_card][d];
                        }
                    }
                }
            }
        }
    }

    readonly property list<string> tabs: [qsTr("Album"), qsTr("Mix"), qsTr("AlbumArtist"), qsTr("Artist"),]
    component LibraryTabBar: Item {
        id: m_library_tab_bar

        readonly property bool textOnly: MD.MProp.size.isCompact

        implicitHeight: textOnly ? m_text_tab_view.implicitHeight : m_chip_tab_view.implicitHeight
        implicitWidth: textOnly ? m_text_tab_view.implicitWidth : m_chip_tab_view.implicitWidth

        MD.HorizontalListView {
            id: m_chip_tab_view
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            implicitHeight: contentItem.childrenRect.height
            spacing: 8
            leftMargin: 8
            rightMargin: 8
            width: Math.min(implicitWidth, parent.width)
            visible: !m_library_tab_bar.textOnly
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

        MD.HorizontalListView {
            id: m_text_tab_view
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            implicitHeight: 32
            spacing: 8
            leftMargin: 4
            rightMargin: 4
            width: Math.min(implicitWidth, parent.width)
            visible: m_library_tab_bar.textOnly
            model: root.tabs

            delegate: T.Button {
                id: m_text_tab

                required property string modelData
                required property int index

                activeFocusOnTab: true
                checkable: false
                checked: index == root.currentIndex
                implicitHeight: 32
                implicitWidth: contentItem.implicitWidth + leftPadding + rightPadding
                leftPadding: 4
                rightPadding: 4
                text: modelData
                onClicked: root.currentIndex = index

                contentItem: MD.Label {
                    color: MD.MProp.textColor
                    horizontalAlignment: Text.AlignHCenter
                    text: m_text_tab.text
                    typescale: m_text_tab.checked ? MD.Token.typescale.title_medium : MD.Token.typescale.label_large
                    verticalAlignment: Text.AlignVCenter
                }
                background: Item {
                    MD.FocusIndicator {
                        active: m_text_tab.visualFocus
                        corners: MD.Util.corners(4)
                    }
                }
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
        id: dg_mix_card
        QA.MixCardDelegate {
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
        id: dg_mixlist
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
        property QA.FilterRuleModel filterModel
        property list<MD.Action> headerActions
        property var sortModel
        property string type
        signal selectDisplayMode(int mode)
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

        required property BaseView view

        horizontalPadding: 8

        QA.SortMenu {
            id: m_header_sort_menu
            y: m_header_bar.height
            model: m_header_bar.view?.sortModel ?? null
        }

        verticalPadding: 4

        readonly property list<MD.Action> preActions: [
            QA.SelectDisplayModeAction {
                displayMode: m_header_bar.view?.displayMode ?? 0
                onSelectDisplayMode: m => m_header_bar.view?.selectDisplayMode(m)
            },
            QA.FilterAction {
                model: m_header_bar.view?.filterModel ?? null
            }
        ]

        contentItem: RowLayout {
            id: m_header_row

            spacing: 4

            LibraryTabBar {
                id: m_library_tabs

                Layout.alignment: Qt.AlignVCenter
                Layout.fillWidth: true
                Layout.minimumWidth: Math.min(implicitWidth, 160)
            }

            QA.OrderChip {
                id: m_order_chip

                Layout.alignment: Qt.AlignVCenter
                Layout.maximumWidth: implicitWidth
                Layout.minimumWidth: implicitWidth
                text: {
                    const m = m_header_bar.view?.sortModel;
                    return m?.item(m.currentIndex)?.name ?? "";
                }
                asc: m_header_bar.view?.sortModel?.asc ?? false
                onClicked: {
                    m_header_sort_menu.open();
                }
            }

            MD.ActionToolBar {
                id: m_tool_bar

                Layout.alignment: Qt.AlignVCenter
                Layout.maximumWidth: maximumContentWidth
                Layout.preferredWidth: Math.min(maximumContentWidth, Math.max(Layout.minimumWidth, m_header_row.width - m_library_tabs.implicitWidth - m_order_chip.implicitWidth - m_header_row.spacing * 2))
                actions: m_header_bar.view ? [...m_header_bar.view.headerActions, ...m_header_bar.preActions] : m_header_bar.preActions
                iconDelegate: MD.SmallIconButton {
                    id: m_item
                    anchors.verticalCenter: parent.verticalCenter
                    action: MD.ToolBarLayout.action as MD.Action
                    Component.onCompleted: {
                        if (action.hasOwnProperty('menuParent')) {
                            action.menuParent = m_item;
                        }
                    }
                }
                moreDelegate: MD.SmallIconButton {
                    action: m_tool_bar.moreAction
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
            q.filterLogics = this.filterLogics;
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
            q.filterLogics = this.filterLogics;
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
            q.filterLogics = this.filterLogics;
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

    QA.MixFilterRuleModel {
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
