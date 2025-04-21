import QtQml.Models
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root

    readonly property bool canBack: leaf.folded && leaf.rightAbove
    title: m_content.currentItem?.title ?? qsTr("library")
    property int vpadding: showHeader ? 0 : MD.MProp.size.verticalPadding
    scrolling: m_content.currentItem?.scrolling ?? false

    function back() {
        m_content.pop(null);
    }
    function refresh_list(qr) {
        const old_limit = qr.limit;
        qr.limit = 0;
        qr.offset = 0;
        qr.limit = Math.max(old_limit, qr.data.rowCount());
    }

    QA.Leaflet {
        id: leaf
        anchors.fill: parent
        rightAbove: m_content.depth === 2
        leftMin: 280
        rightMin: 400

        leftPage: MD.Pane {
            topPadding: root.vpadding
            showBackground: false

            ColumnLayout {
                id: p1
                anchors.fill: parent
                spacing: 0

                MD.TabBar {
                    id: bar
                    Layout.fillWidth: true
                    corners: MD.Util.corner(root.radius, 0)

                    Component.onCompleted: {
                        currentIndexChanged();
                    }

                    MD.TabButton {
                        text: qsTr("Album")
                    }
                    MD.TabButton {
                        text: qsTr("Mix")
                    }
                    MD.TabButton {
                        width: implicitWidth
                        text: qsTr("AlbumArtist")
                    }
                    MD.TabButton {
                        text: qsTr("Artist")
                    }

                }
                StackLayout {
                    currentIndex: bar.currentIndex

                    BaseView {
                        id: view_albumlist
                        busy: qr_albums.querying
                        delegate: dg_albumlist
                        model: qr_albums.data
                        type: 'album'
                        refresh: function () {
                            root.refresh_list(qr_albums);
                        }
                    }

                    BaseView {
                        id: m_view_mix
                        busy: qr_mix.querying
                        delegate: dg_playlist
                        model: qr_mix.data
                        refresh: function () {
                            root.refresh_list(qr_mix);
                        }
                        // MD.FAB {
                        //     anchors.right: parent.right
                        //     anchors.bottom: parent.bottom
                        //     anchors.rightMargin: 16
                        //     anchors.bottomMargin: 16
                        //     flickable: m_view_mix
                        //     action:MD.Action {
                        //         icon.name: MD.Token.icon.add
                        //         onTriggered: {
                        //             MD.Util.showPopup('qrc:/Qcm/App/qml/dialog/MixCreateDialog.qml', {}, root.Overlay.overlay);
                        //         }
                        //     }
                        // }
                    }
                    BaseView {
                        id: m_view_album_artists
                        delegate: dg_artistlist
                        busy: qr_album_artists.querying
                        model: qr_album_artists.data
                        type: 'artist'
                        refresh: function () {
                            root.refresh_list(qr_album_artists);
                        }
                    }

                    BaseView {
                        id: view_artistlist
                        delegate: dg_artistlist
                        busy: qr_artists.querying
                        model: qr_artists.data
                        type: 'artist'
                        refresh: function () {
                            root.refresh_list(qr_artists);
                        }
                    }
                    /*




                    BaseView {
                        id: view_djradiolist
                        busy: qr_djradiolist.status === QA.Enum.Querying
                        delegate: dg_djradiolist
                        model: qr_djradiolist.data
                        refresh: function () {
                            root.refresh_list(qr_djradiolist);
                        }
                        Connections {
                            function onDjradioLiked() {
                                view_djradiolist.dirty = true;
                            }

                            target: QA.App
                        }
                    }
                    */
                }
                QA.AlbumsQuery {
                    id: qr_albums
                    Component.onCompleted: reload()
                }
                QA.ArtistsQuery {
                    id: qr_artists
                    Component.onCompleted: reload()
                }
                QA.AlbumArtistsQuery {
                    id: qr_album_artists
                    Component.onCompleted: reload()
                }
                QA.MixesQuery {
                    id: qr_mix
                    Component.onCompleted: reload()
                }
                /*
                QA.RadioCollectionQuery {
                    id: qr_djradiolist
                    Component.onCompleted: reload()
                }
                */
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
                        function showMenu(parent) {
                            console.error(ListView.view.model.extra(index).artists);
                        // MD.Util.showPopup('qrc:/Qcm/App/qml/menu/AlbumMenu.qml', {
                        //     "itemId": model.itemId,
                        //     "y": parent.height
                        // }, parent);
                        }
                    }
                }
                Component {
                    id: dg_artistlist
                    BaseItem {
                        image: QA.Util.image_url(model.itemId)
                        text: model.name
                        // supportText: `${model.albumCount} albums`
                        function showMenu(parent) {
                            MD.Util.showPopup('qrc:/Qcm/App/qml/menu/ArtistMenu.qml', {
                                "itemId": model.itemId,
                                "y": parent.height
                            }, parent);
                        }
                    }
                }
                Component {
                    id: dg_playlist
                    BaseItem {
                        image: QA.Util.image_url(model.itemId)
                        text: model.name
                        supportText: `${model.trackCount} songs`
                        function showMenu(parent) {
                            MD.Util.showPopup('qrc:/Qcm/App/qml/menu/MixMenu.qml', {
                                "itemId": model.itemId,
                                "userId": model.userId,
                                "y": parent.height
                            }, parent);
                        }
                    }
                }
                Component {
                    id: dg_djradiolist
                    BaseItem {
                        image: QA.Util.image_url(model.itemId)
                        text: model.name
                        supportText: `${model.programCount} programs`
                        function showMenu(parent) {
                            MD.Util.showPopup('qrc:/Qcm/App/qml/menu/RadioMenu.qml', {
                                "itemId": model.itemId,
                                "y": parent.height
                            }, parent);
                        }
                    }
                }
            }
        }
        rightPage: MD.StackView {
            id: m_content

            property var currentItemId: null

            MD.MProp.page: m_page_context
            MD.PageContext {
                id: m_page_context
                showHeader: false
                radius: root.radius
            }

            function push_page(item, params, oper) {
                if (m_content.depth === 1)
                    m_content.push(item, params, oper);
                else
                    m_content.replace(m_content.currentItem, item, params, oper);
            }
            function route(itemId) {
                currentItemId = itemId;
                let url = itemId.pageUrl;
                push_page(url, {
                    "itemId": itemId
                });
            }

            popExit: null
            pushExit: null
            replaceExit: null

            initialItem: Item {}
        }
    }

    component BaseView: MD.VerticalListView {
        id: m_view_base
        bottomMargin: root.vpadding

        property bool dirty: false
        property string type
        property var refresh: function () {}

        function checkCur() {
            if (currentItem) {
                if (currentItem.itemId !== m_content.currentItemId)
                    currentIndex = -1;
            }
        }
        function checkDirty() {
            if (visible && dirty) {
                refresh();
                dirty = false;
            }
        }

        Timer {
            id: timer_dirty
            repeat: false
            interval: 1000
            onTriggered: parent.checkDirty()
        }

        currentIndex: -1
        highlightMoveDuration: 1000
        highlightMoveVelocity: -1

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
            MD.ListBusyFooter {
                Layout.fillWidth: true
                running: parent.ListView.view.busy
            }
        }

        Component.onCompleted: {
            visibleChanged.connect(checkCur);
            currentItemChanged.connect(checkCur);
            visibleChanged.connect(checkDirty);
            dirtyChanged.connect(timer_dirty.restart);
        }
    }

    component BaseItem: MD.ListItem {
        required property int index
        required property var model
        property var itemId: model.itemId
        property string image

        width: ListView.view.width
        maximumLineCount: 2

        corners: MD.Util.corner(0, dgIndex + 1 == count ? root.radius : 0)

        leader: QA.Image {
            radius: 8
            source: image
            implicitWidth: displaySize.width
            implicitHeight: displaySize.height

            displaySize: Qt.size(48, 48)
        }
        rightPadding: 0
        trailing: MD.IconButton {
            MD.MProp.textColor: MD.Token.color.on_surface_variant
            icon.name: MD.Token.icon.more_vert
            onClicked: {
                if (showMenu)
                    showMenu(this);
            }
        }
        divider: MD.Divider {
            anchors.bottom: parent.bottom
            leftMargin: 48 + 16 * 2
        }
        onClicked: {
            m_content.route(itemId);

            ListView.view.currentIndex = index;
        }
    }
}
