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
                }
                StackLayout {
                    currentIndex: bar.currentIndex

                    BaseView {
                        id: view_playlist
                        delegate: dg_playlist
                        model: qr_playlist.data
                        refresh: function () {
                            root.refresh_list(qr_playlist);
                        }

                        Connections {
                            function onPlaylistLiked() {
                                view_playlist.dirty = true;
                            }

                            target: QA.App
                        }
                    }
                    BaseView {
                        id: view_albumlist
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
                        delegate: dg_artistlist
                        model: qr_artistlist.data
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
                }
                Component {
                    id: dg_albumlist
                    MD.ListItem {
                        property var itemId: model.itemId

                        width: ListView.view.width
                        text: model.name
                        maximumLineCount: 2
                        supportText: QA.Global.join_name(model.artists, '/')
                        leader: MD.Image {
                            radius: 8
                            source: `image://ncm/${model.picUrl}`
                            sourceSize.height: 48
                            sourceSize.width: 48
                        }
                        onClicked: {
                            content.route(itemId);
                            ListView.view.currentIndex = index;
                        }
                    }
                }
                Component {
                    id: dg_artistlist
                    MD.ListItem {
                        property var itemId: model.itemId

                        width: ListView.view.width
                        text: model.name
                        maximumLineCount: 2
                        supportText: `${model.albumSize} albums`
                        leader: MD.Image {
                            radius: 24
                            source: `image://ncm/${model.picUrl}`
                            sourceSize.height: 48
                            sourceSize.width: 48
                        }
                        onClicked: {
                            content.route(itemId);
                            ListView.view.currentIndex = index;
                        }
                    }
                }
                Component {
                    id: dg_playlist
                    MD.ListItem {
                        property var itemId: model.itemId

                        width: ListView.view.width
                        text: model.name
                        maximumLineCount: 2
                        supportText: `${model.trackCount} songs`
                        leader: MD.Image {
                            radius: 8
                            source: `image://ncm/${model.picUrl}`
                            sourceSize.height: 48
                            sourceSize.width: 48
                        }
                        onClicked: {
                            content.route(itemId);
                            ListView.view.currentIndex = index;
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
                push_page(QA.Global.item_id_url(itemId), {
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
}
