import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App
import Qcm.Material as MD
import ".."
import "../component"
import "../part"

Page {
    id: root

    readonly property bool canBack: leaf.folded && leaf.rightAbove

    function back() {
        content.pop(null);
    }

    Material.background: Theme.color.surface

    Leaflet {
        id: leaf
        anchors.fill: parent
        leftMin: 280
        rightAbove: content.depth === 2
        rightMin: 400

        leftPage: Pane {
            Material.elevation: 2
            padding: 0

            ColumnLayout {
                id: p1
                anchors.fill: parent

                TabBar {
                    id: bar
                    Layout.fillWidth: true
                    Material.elevation: 1

                    Component.onCompleted: {
                        currentIndexChanged();
                    }

                    TabButton {
                        text: qsTr("Playlist")
                    }
                    TabButton {
                        text: qsTr("Album")
                    }
                    TabButton {
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
                            api_container.refresh_list(qr_playlist);
                        }

                        Connections {
                            function onSig_like_playlist() {
                                view_playlist.dirty = true;
                            }

                            target: QA
                        }
                    }
                    BaseView {
                        id: view_albumlist
                        delegate: dg_albumlist
                        model: qr_albumlist.data
                        refresh: function () {
                            api_container.refresh_list(qr_albumlist);
                        }

                        Connections {
                            function onSig_like_album() {
                                view_albumlist.dirty = true;
                            }

                            target: QA
                        }
                    }
                    BaseView {
                        delegate: dg_artistlist
                        model: qr_artistlist.data
                    }
                }
                ApiContainer {
                    id: api_container
                    function refresh_list(qr) {
                        const old_limit = qr.limit;
                        qr.limit = 0;
                        qr.offset = 0;
                        qr.limit = Math.max(old_limit, qr.data.rowCount());
                    }

                    AlbumSublistQuerier {
                        id: qr_albumlist
                        autoReload: limit > 0
                    }
                    ArtistSublistQuerier {
                        id: qr_artistlist
                        autoReload: limit > 0
                    }
                    UserPlaylistQuerier {
                        id: qr_playlist
                        autoReload: uid.valid() && limit > 0
                        uid: QA.user_info.userId
                    }
                }
                Component {
                    id: dg_albumlist
                    MD.ListItem {
                        property var itemId: model.itemId

                        width: ListView.view.width
                        text: model.name
                        maximumLineCount: 2
                        supportText: QA.join_name(model.artists, '/')
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
        rightPage: StackView {
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
                push_page(QA.item_id_url(itemId), {
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

    component BaseView: MListView {
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

        ScrollBar.vertical: ScrollBar {
        }

        Component.onCompleted: {
            visibleChanged.connect(checkCur);
            currentItemChanged.connect(checkCur);
            visibleChanged.connect(checkDirty);
            dirtyChanged.connect(visibleChanged);
        }
    }
}
