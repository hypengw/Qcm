import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QcmApp
import ".."
import "../component"
import "../part"

Page {
    id: root

    readonly property bool canBack: leaf.folded && leaf.rightAbove

    function back() {
        content.pop(null);
    }

    Leaflet {
        id: leaf

        anchors.fill: parent
        leftMin: 300
        rightMin: 360
        rightAbove: content.depth === 2

        leftPage: Pane {
            Material.elevation: 2
            padding: 0

            ColumnLayout {
                id: p1

                anchors.fill: parent

                TabBar {
                    id: bar

                    Layout.fillWidth: true
                    Material.elevation: 2
                    Material.background: Theme.color.surface_2
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

                        model: qr_playlist.data
                        delegate: dg_playlist
                        refresh: function() {
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

                        model: qr_albumlist.data
                        delegate: dg_albumlist
                        refresh: function() {
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
                        model: qr_artistlist.data
                        delegate: dg_artistlist
                    }

                    component BaseView: MListView {
                        property var refresh: function() {
                        }
                        property bool dirty: false

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

                        currentIndex: -1
                        clip: true
                        highlightMoveDuration: 1000
                        highlightMoveVelocity: -1
                        Component.onCompleted: {
                            visibleChanged.connect(checkCur);
                            currentItemChanged.connect(checkCur);
                            visibleChanged.connect(checkDirty);
                            dirtyChanged.connect(visibleChanged);
                        }

                        ScrollBar.vertical: ScrollBar {
                        }

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

                        uid: QA.user_info.userId
                        autoReload: uid.valid() && limit > 0
                    }

                }

                Component {
                    id: dg_albumlist

                    MItemDelegate {
                        property var itemId: model.itemId

                        width: ListView.view.width
                        onClicked: {
                            content.route(itemId);
                            ListView.view.currentIndex = index;
                        }

                        contentItem: RowLayout {
                            width: parent.width
                            spacing: 8

                            Image {
                                sourceSize.width: 48
                                sourceSize.height: 48
                                source: `image://ncm/${model.picUrl}`
                            }

                            ColumnLayout {
                                Label {
                                    Layout.fillWidth: true
                                    maximumLineCount: 4
                                    wrapMode: Text.Wrap
                                    elide: Text.ElideRight
                                    text: model.name
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: QA.join_name(model.artists, '/')
                                    elide: Text.ElideRight
                                    opacity: 0.6
                                    font.pointSize: Theme.ts.label_small.size
                                }

                            }

                        }

                    }

                }

                Component {
                    id: dg_artistlist

                    MItemDelegate {
                        property var itemId: model.itemId

                        width: ListView.view.width
                        onClicked: {
                            content.route(itemId);
                            ListView.view.currentIndex = index;
                        }

                        contentItem: RowLayout {
                            width: parent.width
                            spacing: 8

                            RoundImage {

                                image: Image {
                                    sourceSize.width: 48
                                    sourceSize.height: 48
                                    source: `image://ncm/${model.picUrl}`
                                }

                            }

                            ColumnLayout {
                                Label {
                                    Layout.fillWidth: true
                                    maximumLineCount: 4
                                    wrapMode: Text.Wrap
                                    elide: Text.ElideRight
                                    text: model.name
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: `${model.albumSize} albums`
                                    elide: Text.ElideRight
                                    opacity: 0.6
                                    font.pointSize: Theme.font.small(Theme.font.label_font)
                                }

                            }

                        }

                    }

                }

                Component {
                    id: dg_playlist

                    MItemDelegate {
                        property var itemId: model.itemId

                        width: ListView.view.width
                        onClicked: {
                            content.route(itemId);
                            ListView.view.currentIndex = index;
                        }

                        contentItem: RowLayout {
                            width: parent.width
                            spacing: 8

                            Image {
                                sourceSize.width: 48
                                sourceSize.height: 48
                                source: `image://ncm/${model.picUrl}`
                            }

                            ColumnLayout {
                                Label {
                                    Layout.fillWidth: true
                                    maximumLineCount: 4
                                    wrapMode: Text.Wrap
                                    elide: Text.ElideRight
                                    text: model.name
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: `${model.trackCount} songs`
                                    elide: Text.ElideRight
                                    opacity: 0.6
                                    font.pointSize: Theme.font.small(Theme.font.label_font)
                                }

                            }

                        }

                    }

                }

            }

        }

        rightPage: StackView {
            id: content

            property var currentItemId: null

            function route(itemId) {
                currentItemId = itemId;
                push_page(QA.item_id_url(itemId), {
                    "itemId": itemId
                });
            }

            function push_page(item, params, oper) {
                if (content.depth === 1)
                    content.push(item, params, oper);
                else
                    content.replace(content.currentItem, item, params, oper);
            }

            pushExit: null
            popExit: null
            replaceExit: null

            initialItem: Item {
            }

        }

    }

}
