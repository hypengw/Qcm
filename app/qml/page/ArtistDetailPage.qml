import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Page {
    id: root

    property alias itemData: qr_artist.data
    property alias itemId: qr_artist.itemId

    title: qsTr("artist")
    padding: 0

    MD.Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: content.implicitHeight
        ScrollBar.vertical.visible: false

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

                QA.Image {
                    z: 1
                    elevation: MD.Token.elevation.level2
                    source: `image://ncm/${root.itemData.info.picUrl}`
                    radius: width / 2

                    Layout.preferredWidth: displaySize.width
                    Layout.preferredHeight: displaySize.height
                    displaySize: Qt.size(240, 240)
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
                    action: QA.SubAction {
                        liked: root.itemData.info.followed
                        querier: qr_sub
                        itemId: root.itemId
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
                            implicitHeight: contentHeight
                            interactive: flick.atYEnd
                            model: itemData.hotSongs
                            topMargin: 8
                            bottomMargin: 8
                            leftMargin: 24
                            rightMargin: 24

                            onAtYBeginningChanged: {
                                if (interactive) {
                                    flick.contentY -= 1;
                                }
                            }

                            delegate: QA.SongDelegate {
                                subtitle: `${modelData.album.name}`
                                width: ListView.view.contentWidth

                                onClicked: {
                                    QA.App.playlist.switchTo(modelData);
                                }
                            }
                            footer: MD.ListBusyFooter {
                                running: qr_artist.status === QA.enums.Querying
                                width: ListView.view.width
                            }
                        }
                        QA.MGridView {
                            fixedCellWidth: Math.max(160, QA.Global.main_win.width / 6.0)
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

                                    picWidth: parent.GridView.view.fixedCellWidth
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
    QNcm.ArtistQuerier {
        id: qr_artist
        autoReload: itemId.valid()
    }
    QNcm.ArtistSubQuerier {
        id: qr_sub
        autoReload: false
        onStatusChanged: {
            if (status === QA.enums.Finished) {
                QA.App.artistLiked(itemId, sub);
            }
        }
    }
    QNcm.ArtistAlbumsQuerier {
        id: qr_artist_albums
        artistId: qr_artist.itemId
        autoReload: artistId.valid()
    }
}
