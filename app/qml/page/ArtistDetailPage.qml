import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root

    property alias itemData: qr_artist.data
    property alias itemId: qr_artist.itemId

    padding: 0

    MD.Flickable {
        id: flick
        anchors.fill: parent
        clip: true
        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 8

            RowLayout {
                id: ly_header
                spacing: 16
                Layout.leftMargin: 8
                Layout.rightMargin: 8
                Layout.topMargin: 8

                MD.Image {
                    MD.MatProp.elevation: MD.Token.elevation.level2
                    source: `image://ncm/${root.itemData.info.picUrl}`
                    sourceSize.height: 240
                    sourceSize.width: 240
                    radius: width / 2
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignTop
                    spacing: 12

                    MD.Text {
                        Layout.fillWidth: true
                        maximumLineCount: 2
                        text: root.itemData.info.name
                        typescale: MD.Token.typescale.headline_large
                    }
                    RowLayout {
                        spacing: 12
                        MD.Text {
                            typescale: MD.Token.typescale.body_medium
                            text: `${root.itemData.info.albumSize} albums`
                        }
                        MD.Text {
                            typescale: MD.Token.typescale.body_medium
                            text: `${root.itemData.info.musicSize} songs`
                        }
                    }
                    QA.ListDescription {
                        description: root.itemData.info.briefDesc.trim()
                        Layout.fillWidth: true
                    }
                }
            }
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                MD.IconButton {
                    id: btn_fav
                    property bool liked: root.itemData.info.followed
                    action: Action {
                        icon.name: btn_fav.liked ? MD.Token.icon.done : MD.Token.icon.add
                        onTriggered: {
                            qr_sub.sub = !btn_fav.liked;
                            qr_sub.itemId = root.itemId;
                            qr_sub.query();
                        }
                    }
                    Binding on liked {
                        value: qr_sub.sub
                        when: qr_sub.status === QA.ApiQuerierBase.Finished
                    }
                }
            }

            ColumnLayout {
                id: pane_view_column
                spacing: 0

                MD.TabBar {
                    id: bar
                    Layout.fillWidth: true

                    Component.onCompleted: {
                        currentIndexChanged();
                    }

                    MD.TabButton {
                        text: qsTr("Hot Song")
                    }
                    MD.TabButton {
                        text: qsTr("Album")
                    }
                }
                MD.Pane {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    implicitHeight: Math.min(Math.max(root.height - ly_header.implicitHeight * 0.4 - bar.implicitHeight, 0), item_stack.implicitHeight)
                    padding: 0

                    MD.MatProp.backgroundColor: MD.Token.color.surface

                    StackLayout {
                        id: item_stack
                        anchors.fill: parent
                        currentIndex: bar.currentIndex

                        MD.ListView {
                            clip: true
                            implicitHeight: contentHeight
                            interactive: flick.atYEnd
                            model: itemData.hotSongs
                            onAtYBeginningChanged: {
                                if (interactive) {
                                    flick.contentY -= 1;
                                }
                            }

                            delegate: QA.SongDelegate {
                                count: ListView.view.count
                                subtitle: `${modelData.album.name}`
                                width: ListView.view.width

                                onClicked: {
                                    QA.Global.playlist.switchTo(modelData);
                                }
                            }
                            footer: MD.ListBusyFooter {
                                running: qr_artist.status === QA.ApiQuerierBase.Querying
                                width: ListView.view.width
                            }
                        }
                        QA.MGridView {
                            cellHeight: 264

                            clip: true
                            interactive: flick.atYEnd
                            model: qr_artist_albums.data
                            onAtYBeginningChanged: {
                                if (interactive) {
                                    flick.contentY -= 1;
                                }
                            }

                            delegate: Item {
                                width: GridView.view.cellWidth
                                height: GridView.view.cellHeight
                                QA.PicGridDelegate {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.top: parent.top
                                    anchors.topMargin: 8

                                    width: picWidth
                                    height: Math.min(implicitHeight, parent.height)
                                    image.source: `image://ncm/${model.picUrl}`
                                    text: model.name
                                    subText: Qt.formatDateTime(model.publishTime, 'yyyy')

                                    onClicked: {
                                        QA.Global.route(model.itemId);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    QA.ArtistQuerier {
        id: qr_artist
        autoReload: itemId.valid()
    }
    QA.ArtistSubQuerier {
        id: qr_sub
        autoReload: false
        onStatusChanged: {
            if (status === QA.ApiQuerierBase.Finished) {
                QA.App.artistLiked(itemId, sub);
            }
        }
    }
    QA.ArtistAlbumsQuerier {
        id: qr_artist_albums
        artistId: qr_artist.itemId
        autoReload: artistId.valid()
    }
}
