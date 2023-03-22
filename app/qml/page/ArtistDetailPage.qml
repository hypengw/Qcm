import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QcmApp
import ".."
import "../component"
import "../part"

Page {
    id: root

    property alias itemId: qr_artist.itemId
    property alias itemData: qr_artist.data

    padding: 16
    verticalPadding: 0

    Flickable {
        id: flick

        anchors.horizontalCenter: parent.horizontalCenter
        topMargin: 16
        bottomMargin: 16
        width: Math.min(800, parent.width)
        height: parent.height
        contentWidth: width
        contentHeight: content.implicitHeight
        clip: false
        boundsBehavior: Flickable.StopAtBounds

        ColumnLayout {
            id: content

            anchors.fill: parent
            spacing: 4

            Pane {
                Layout.fillWidth: true
                padding: 0

                ColumnLayout {
                    anchors.fill: parent

                    Pane {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        RowLayout {
                            anchors.fill: parent
                            spacing: 16

                            MPane {
                                Layout.alignment: Qt.AlignTop
                                Layout.preferredWidth: 160 + 2 * padding
                                Layout.preferredHeight: Layout.preferredWidth
                                Material.elevation: 2
                                Material.background: Theme.color.surface_2
                                padding: 4
                                radius: width / 2

                                RoundImage {

                                    image: Image {
                                        source: `image://ncm/${root.itemData.info.picUrl}`
                                        sourceSize.width: 160
                                        sourceSize.height: 160
                                    }

                                }

                            }

                            ColumnLayout {
                                Layout.alignment: Qt.AlignTop
                                spacing: 12

                                Label {
                                    Layout.fillWidth: true
                                    text: root.itemData.info.name
                                    maximumLineCount: 2
                                    wrapMode: Text.Wrap
                                    elide: Text.ElideRight
                                    font.pointSize: 20
                                    font.bold: true
                                }

                                IconRowLayout {
                                    text: Theme.ic.album
                                    iconSize: 16

                                    Label {
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                        text: `${root.itemData.info.albumSize} albums`
                                    }

                                }

                                IconRowLayout {
                                    text: Theme.ic.music_note
                                    iconSize: 16

                                    Label {
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                        text: `${root.itemData.info.musicSize} songs`
                                    }

                                }

                                Item {
                                    Layout.fillHeight: true
                                }

                                IconRowLayout {
                                    Layout.alignment: Qt.AlignBottom
                                    text: Theme.ic.info
                                    iconSize: 16

                                    Label {
                                        id: ttt

                                        Layout.fillWidth: true
                                        maximumLineCount: 2
                                        wrapMode: Text.Wrap
                                        elide: Text.ElideRight
                                        textFormat: Text.PlainText
                                        text: `${root.itemData.info.briefDesc}`.trim()
                                    }

                                }

                            }

                        }

                    }

                }

            }

            Pane {
                Layout.fillWidth: true
                implicitHeight: Math.min(root.height * 0.75, pane_view_column.implicitHeight)
                padding: 0

                ColumnLayout {
                    id: pane_view_column

                    anchors.fill: parent
                    spacing: 0

                    Pane {
                        padding: 0
                        Layout.fillWidth: true
                        Material.elevation: 1

                        TabBar {
                            id: bar

                            anchors.fill: parent
                            Material.elevation: 0
                            Material.background: Theme.color.surface_2
                            Component.onCompleted: {
                                currentIndexChanged();
                            }

                            TabButton {
                                text: qsTr("Hot Song")
                            }

                            TabButton {
                                text: qsTr("Album")
                            }

                        }

                    }

                    Pane {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Material.elevation: 1
                        Material.background: Theme.color.surface_1
                        padding: 0

                        StackLayout {
                            currentIndex: bar.currentIndex
                            anchors.fill: parent

                            ListView {
                                implicitHeight: contentHeight
                                boundsBehavior: Flickable.StopAtBounds
                                interactive: flick.atYEnd
                                clip: true
                                model: itemData.hotSongs

                                delegate: SongDelegate {
                                    width: ListView.view.width
                                    count: ListView.view.count
                                    subtitle: `${modelData.album.name}`
                                    onClicked: {
                                        QA.playlist.switchTo(modelData);
                                    }
                                }

                                footer: ListBusyFooter {
                                    width: ListView.view.width
                                    running: qr_artist.status === ApiQuerierBase.Querying
                                }

                                ScrollBar.vertical: ScrollBar {
                                }

                            }

                            GridView {
                                property int cellWidth_: 180

                                boundsBehavior: Flickable.StopAtBounds
                                interactive: flick.atYEnd
                                clip: true
                                model: qr_artist_albums.data
                                cellHeight: 250
                                cellWidth: width > 0 ? width / Math.floor((width / cellWidth_)) : 0

                                delegate: Pane {
                                    width: GridView.view.cellWidth
                                    height: GridView.view.cellHeight

                                    MItemDelegate {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        clip: true
                                        padding: 8
                                        onClicked: {
                                            QA.route(model.itemId);
                                        }

                                        contentItem: ColumnLayout {
                                            Image {
                                                Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                                                Layout.preferredWidth: 160
                                                Layout.preferredHeight: 160
                                                source: `image://ncm/${model.picUrl}`
                                                sourceSize.width: 160
                                                sourceSize.height: 160
                                            }

                                            Label {
                                                Layout.topMargin: 8
                                                Layout.preferredWidth: 160
                                                text: model.name
                                                maximumLineCount: 2
                                                wrapMode: Text.Wrap
                                                elide: Text.ElideRight
                                            }

                                            Label {
                                                visible: !!text
                                                Layout.alignment: Qt.AlignHCenter
                                                text: Qt.formatDateTime(model.publishTime, 'yyyy')
                                                opacity: 0.6
                                            }

                                            Item {
                                                Layout.fillHeight: true
                                            }

                                        }

                                    }

                                }

                                ScrollBar.vertical: ScrollBar {
                                }

                            }

                        }

                    }

                }

            }

        }

    }

    ApiContainer {
        ArtistQuerier {
            id: qr_artist

            autoReload: itemId.valid()
        }

        ArtistAlbumsQuerier {
            id: qr_artist_albums

            artistId: qr_artist.itemId
            autoReload: artistId.valid()
        }

    }

}
