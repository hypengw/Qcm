import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root

    readonly property bool canBack: leaf.folded && leaf.rightAbove

    function back() {
        content.pop(null);
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
        leftMin: 280
        rightAbove: content.depth === 2
        rightMin: 400

        leftPage: MD.Pane {
            padding: 0

            ColumnLayout {
                id: p1
                anchors.fill: parent
                spacing: 0

                MD.TabBar {
                    id: bar
                    Layout.fillWidth: true

                    Component.onCompleted: {
                        currentIndexChanged();
                    }

                    MD.TabButton {
                        text: qsTr("Playlist")
                    }
                    MD.TabButton {
                        text: qsTr("Album")
                    }
                    MD.TabButton {
                        text: qsTr("Artist")
                    }
                    MD.TabButton {
                        text: qsTr("Djradio")
                    }
                }
                StackLayout {
                    currentIndex: bar.currentIndex

                    BaseView {
                        id: view_playlist
                        busy: qr_playlist.status === QA.ApiQuerierBase.Querying
                        delegate: dg_playlist
                        model: qr_playlist.data
                        refresh: function () {
                            root.refresh_list(qr_playlist);
                        }

                        Connections {
                            function onPlaylistChanged() {
                                view_playlist.dirty = true;
                            }
                            function onPlaylistDeleted() {
                                view_playlist.dirty = true;
                            }
                            function onPlaylistCreated() {
                                view_playlist.dirty = true;
                            }
                            function onPlaylistLiked() {
                                view_playlist.dirty = true;
                            }
                            function onSongLiked() {
                                view_playlist.dirty = true;
                            }

                            target: QA.App
                        }
                        MD.FAB {
                            action: Action {
                                icon.name: MD.Token.icon.add
                                onTriggered: {
                                    QA.Global.show_page_popup('qrc:/Qcm/App/qml/page/PlaylistCreatePage.qml', {});
                                }
                            }
                        }
                    }
                    BaseView {
                        id: view_albumlist
                        busy: qr_albumlist.status === QA.ApiQuerierBase.Querying
                        delegate: dg_albumlist
                        model: qr_albumlist.data
                        refresh: function () {
                            root.refresh_list(qr_albumlist);
                        }
                        Connections {
                            function onAlbumLiked() {
                                view_albumlist.dirty = true;
                            }

                            target: QA.App
                        }
                    }
                    BaseView {
                        id: view_artistlist
                        delegate: dg_artistlist
                        busy: qr_artistlist.status === QA.ApiQuerierBase.Querying
                        model: qr_artistlist.data
                        refresh: function () {
                            root.refresh_list(qr_artistlist);
                        }
                        Connections {
                            function onArtistLiked() {
                                view_artistlist.dirty = true;
                            }

                            target: QA.App
                        }
                    }
                    BaseView {
                        id: view_djradiolist
                        busy: qr_djradiolist.status === QA.ApiQuerierBase.Querying
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
                }
                QA.AlbumSublistQuerier {
                    id: qr_albumlist
                    autoReload: limit > 0
                }
                QA.ArtistSublistQuerier {
                    id: qr_artistlist
                    autoReload: limit > 0
                }
                QA.UserPlaylistQuerier {
                    id: qr_playlist
                    autoReload: uid.valid() && limit > 0
                    uid: QA.Global.user_info.userId
                    limit: 30
                }
                QA.DjradioSublistQuerier {
                    id: qr_djradiolist
                    autoReload: limit > 0
                }
                Component {
                    id: dg_albumlist
                    BaseItem {
                        image: `image://ncm/${model.picUrl}`
                        text: model.name
                        supportText: `${QA.Global.join_name(model.artists, '/')} - ${model.trackCount} tracks`
                        function showMenu(parent) {
                            QA.Global.show_popup('qrc:/Qcm/App/qml/menu/AlbumMenu.qml', {
                                    "album": QA.App.album(model),
                                    "y": parent.height
                                }, parent);
                        }
                    }
                }
                Component {
                    id: dg_artistlist
                    BaseItem {
                        image: `image://ncm/${model.picUrl}`
                        text: model.name
                        supportText: `${model.albumSize} albums`
                        function showMenu(parent) {
                            QA.Global.show_popup('qrc:/Qcm/App/qml/menu/ArtistMenu.qml', {
                                    "artist": QA.App.artist(model),
                                    "y": parent.height
                                }, parent);
                        }
                    }
                }
                Component {
                    id: dg_playlist
                    BaseItem {
                        image: `image://ncm/${model.picUrl}`
                        text: model.name
                        supportText: `${model.trackCount} songs`
                        function showMenu(parent) {
                            QA.Global.show_popup('qrc:/Qcm/App/qml/menu/PlaylistMenu.qml', {
                                    "playlist": QA.App.playlist(model),
                                    "y": parent.height
                                }, parent);
                        }
                    }
                }
                Component {
                    id: dg_djradiolist
                    BaseItem {
                        image: `image://ncm/${model.picUrl}`
                        text: model.name
                        supportText: `${model.programCount} programs`
                        function showMenu(parent) {
                            QA.Global.show_popup('qrc:/Qcm/App/qml/menu/DjradioMenu.qml', {
                                    "djradio": QA.App.djradio(model),
                                    "y": parent.height
                                }, parent);
                        }
                    }
                }
            }
        }
        rightPage: MD.StackView {
            id: content

            property var currentItemId: null

            function push_page(item, params, oper) {
                if (content.depth === 1)
                    content.push(item, params, oper);
                else
                    content.replace(content.currentItem, item, params, oper);
            }
            function route(itemId) {
                currentItemId = itemId;
                push_page(QA.App.itemIdPageUrl(itemId), {
                        "itemId": itemId
                    });
            }

            popExit: null
            pushExit: null
            replaceExit: null

            initialItem: Item {
            }
        }
    }

    component BaseView: MD.ListView {
        property bool dirty: false
        property var refresh: function () {}

        function checkCur() {
            if (currentItem) {
                if (currentItem.itemId !== content.currentItemId)
                    currentIndex = -1;
            }
        }
        function checkDirty() {
            if (visible && dirty) {
                refresh();
                dirty = false;
            }
        }

        clip: true
        currentIndex: -1
        highlightMoveDuration: 1000
        highlightMoveVelocity: -1

        Component.onCompleted: {
            visibleChanged.connect(checkCur);
            currentItemChanged.connect(checkCur);
            visibleChanged.connect(checkDirty);
            dirtyChanged.connect(visibleChanged);
        }
    }

    component BaseItem: MD.ListItem {
        property var itemId: model.itemId
        property string image

        width: ListView.view.width
        maximumLineCount: 2
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
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 48 + 16*2
            full: false
            height: 1
        }
        onClicked: {
            content.route(itemId);
            ListView.view.currentIndex = index;
        }
    }
}
