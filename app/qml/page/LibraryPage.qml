import QtQml.Models
import QtQuick
// import QtQuick.Controls.Basic
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root

    readonly property bool canBack: leaf.folded && leaf.rightAbove
    title: m_content.currentItem?.title ?? qsTr("library")
    property int vpadding: showHeader ? 0 : MD.MatProp.size.verticalPadding
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
                        text: qsTr("Artist")
                    }
                    // MD.TabButton {
                    //     text: qsTr("Mix")
                    // }

                    // MD.TabButton {
                    //     text: qsTr("Radio")
                    // }
                }
                StackLayout {
                    currentIndex: bar.currentIndex

                    BaseView {
                        id: view_albumlist
                        busy: qr_albumlist.querying
                        delegate: dg_albumlist
                        model: qr_albumlist.data
                        type: 'album'
                        refresh: function () {
                            root.refresh_list(qr_albumlist);
                        }
                    }

                    BaseView {
                        id: view_artistlist
                        delegate: dg_artistlist
                        busy: qr_artistlist.querying
                        model: qr_artistlist.data
                        type: 'artist'
                        refresh: function () {
                            root.refresh_list(qr_artistlist);
                        }
                    }
                    /*

                    BaseView {
                        id: m_view_mix
                        busy: qr_playlist.status === QA.Enum.Querying
                        delegate: dg_playlist
                        model: qr_playlist.data
                        refresh: function () {
                            root.refresh_list(qr_playlist);
                        }
                        Connections {
                            function onPlaylistChanged() {
                                m_view_mix.dirty = true;
                            }
                            function onPlaylistDeleted() {
                                m_view_mix.dirty = true;
                            }
                            function onPlaylistCreated() {
                                m_view_mix.dirty = true;
                            }
                            function onPlaylistLiked() {
                                m_view_mix.dirty = true;
                            }

                            target: QA.App
                        }
                        MD.FAB {
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            anchors.rightMargin: 16
                            anchors.bottomMargin: 16
                            flickable: m_view_mix
                            action: Action {
                                icon.name: MD.Token.icon.add
                                onTriggered: {
                                    MD.Util.show_popup('qrc:/Qcm/App/qml/dialog/MixCreateDialog.qml', {}, root.Overlay.overlay)
                                }
                            }
                        }
                    }


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
                    id: qr_albumlist
                    Component.onCompleted: reload()
                }
                QA.ArtistsQuery {
                    id: qr_artistlist
                    Component.onCompleted: reload()
                }
                /*
                QA.MixCollectionQuery {
                    id: qr_playlist
                    Component.onCompleted: reload()
                }
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
                        // MD.Util.show_popup('qrc:/Qcm/App/qml/menu/AlbumMenu.qml', {
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
                            MD.Util.show_popup('qrc:/Qcm/App/qml/menu/ArtistMenu.qml', {
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
                            MD.Util.show_popup('qrc:/Qcm/App/qml/menu/MixMenu.qml', {
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
                            MD.Util.show_popup('qrc:/Qcm/App/qml/menu/RadioMenu.qml', {
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

            MD.MatProp.page: m_page_context
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
                let url = itemId.toPageUrl();
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

    component BaseView: MD.ListView {
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
            MD.MatProp.textColor: MD.Token.color.on_surface_variant
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
