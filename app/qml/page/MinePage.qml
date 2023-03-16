import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QcmApp
import ".."
import "../component"

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

                ListView {
                    id: view

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: {
                        switch (bar.currentIndex) {
                        case 0:
                            return dgm_playlist;
                        case 1:
                            return dgm_albumlist;
                        case 2:
                            return dgm_artistlist;
                        }
                    }
                    highlightMoveDuration: 1000
                    highlightMoveVelocity: -1
                    onCurrentIndexChanged: {
                        if (!currentItem)
                            return ;

                        const itemId = currentItem.itemId;
                        switch (itemId.type) {
                        case ItemId.Album:
                            content.push_page('qrc:/QcmApp/qml/page/AlbumDetailPage.qml', {
                                "itemId": currentItem.itemId
                            });
                            break;
                        case ItemId.Playlist:
                            content.push_page('qrc:/QcmApp/qml/page/PlaylistDetailPage.qml', {
                                "itemId": currentItem.itemId
                            });
                            break;
                        case ItemId.Artist:
                            content.push_page('qrc:/QcmApp/qml/page/ArtistDetailPage.qml', {
                                "itemId": currentItem.itemId
                            });
                            break;
                        }
                    }

                    ScrollBar.vertical: ScrollBar {
                    }

                }

                AlbumSublistQuerier {
                    id: qr_albumlist
                }

                DelegateModel {
                    id: dgm_albumlist

                    model: qr_albumlist.data

                    delegate: MItemDelegate {
                        property var itemId: model.itemId

                        width: view.width
                        onClicked: view.currentIndex = index

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
                                    font.pointSize: Theme.font.small(Theme.font.label_font)
                                }

                            }

                        }

                    }

                }

                ArtistSublistQuerier {
                    id: qr_artistlist
                }

                DelegateModel {
                    id: dgm_artistlist

                    model: qr_artistlist.data

                    delegate: MItemDelegate {
                        property var itemId: model.itemId

                        width: ListView.view.width
                        onClicked: ListView.view.currentIndex = index

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

                UserPlaylistQuerier {
                    id: qr_playlist

                    uid: QA.user_info.userId
                    autoReload: uid.valid()
                }

                DelegateModel {
                    id: dgm_playlist

                    model: qr_playlist.data

                    delegate: MItemDelegate {
                        property var itemId: model.itemId

                        width: ListView.view.width
                        onClicked: ListView.view.currentIndex = index

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

                component HighlightBar: Rectangle {
                    color: Theme.color.primary_container
                }

            }

        }

        rightPage: StackView {
            id: content

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
