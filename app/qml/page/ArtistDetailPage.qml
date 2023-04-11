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

    property alias itemData: qr_artist.data
    property alias itemId: qr_artist.itemId

    padding: 0

    MFlickable {
        id: flick
        anchors.fill: parent
        clip: true
        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content
            anchors.horizontalCenter: parent.horizontalCenter
            height: parent.height
            spacing: 4
            width: Math.min(800, parent.width)

            Pane {
                Layout.fillWidth: true
                padding: 0
                z: 1

                ColumnLayout {
                    anchors.fill: parent

                    Pane {
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        RowLayout {
                            anchors.fill: parent
                            spacing: 16

                            MPane {
                                Layout.alignment: Qt.AlignTop
                                Layout.preferredHeight: Layout.preferredWidth
                                Layout.preferredWidth: 160 + 2 * padding
                                Material.background: Theme.color.surface_2
                                Material.elevation: 2
                                padding: 4
                                radius: width / 2
                                z: 1

                                RoundImage {
                                    image: Image {
                                        source: `image://ncm/${root.itemData.info.picUrl}`
                                        sourceSize.height: 160
                                        sourceSize.width: 160
                                    }
                                }
                            }
                            ColumnLayout {
                                Layout.alignment: Qt.AlignTop
                                spacing: 4

                                Label {
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                    font.bold: true
                                    font.pointSize: Theme.ts.title_medium.size
                                    maximumLineCount: 2
                                    text: root.itemData.info.name
                                    wrapMode: Text.Wrap
                                }
                                InfoRow {
                                    icon_name: Theme.ic.album
                                    label_text: `${root.itemData.info.albumSize} albums`
                                }
                                InfoRow {
                                    icon_name: Theme.ic.music_note
                                    label_text: `${root.itemData.info.musicSize} songs`
                                }
                                Item {
                                    Layout.fillHeight: true
                                }
                                MButton {
                                    id: btn_desc

                                    readonly property string description: `${root.itemData.info.briefDesc}`.trim()

                                    Layout.alignment: Qt.AlignBottom
                                    Layout.fillWidth: true
                                    flat: true
                                    font.pointSize: Theme.ts.label_medium.size
                                    visible: !!btn_desc.description

                                    contentItem: IconRowLayout {
                                        iconSize: 16
                                        text: Theme.ic.info

                                        Label {
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                            maximumLineCount: 2
                                            text: btn_desc.description
                                            textFormat: Text.PlainText
                                            wrapMode: Text.Wrap
                                        }
                                    }

                                    onClicked: {
                                        QA.show_page_popup('qrc:/QcmApp/qml/page/DescriptionPage.qml', {
                                                "text": description
                                            });
                                    }
                                }
                            }
                        }
                    }
                }
            }
            Pane {
                Layout.fillWidth: true
                implicitHeight: Math.min(root.height * 0.75 + pane_tab_bar.implicitHeight, pane_view_column.implicitHeight)
                padding: 0

                ColumnLayout {
                    id: pane_view_column
                    anchors.fill: parent
                    spacing: 0

                    Pane {
                        id: pane_tab_bar
                        Layout.fillWidth: true
                        padding: 0

                        TabBar {
                            id: bar
                            Material.elevation: 0
                            anchors.fill: parent

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
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        Material.elevation: 0
                        padding: 0

                        StackLayout {
                            anchors.fill: parent
                            currentIndex: bar.currentIndex

                            MListView {
                                boundsBehavior: Flickable.StopAtBounds
                                clip: true
                                implicitHeight: contentHeight
                                interactive: flick.atYEnd
                                model: itemData.hotSongs

                                ScrollBar.vertical: ScrollBar {
                                }
                                delegate: SongDelegate {
                                    count: ListView.view.count
                                    subtitle: `${modelData.album.name}`
                                    width: ListView.view.width

                                    onClicked: {
                                        QA.playlist.switchTo(modelData);
                                    }
                                }
                                footer: ListBusyFooter {
                                    running: qr_artist.status === ApiQuerierBase.Querying
                                    width: ListView.view.width
                                }
                            }
                            MGridView {
                                property int cellWidth_: 180

                                boundsBehavior: Flickable.StopAtBounds
                                cellHeight: 250
                                cellWidth: width > 0 ? width / Math.floor((width / cellWidth_)) : 0
                                clip: true
                                interactive: flick.atYEnd
                                model: qr_artist_albums.data

                                ScrollBar.vertical: ScrollBar {
                                }
                                delegate: Pane {
                                    height: GridView.view.cellHeight
                                    width: GridView.view.cellWidth

                                    MItemDelegate {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        clip: true
                                        padding: 8

                                        contentItem: ColumnLayout {
                                            Image {
                                                Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                                                Layout.preferredHeight: 160
                                                Layout.preferredWidth: 160
                                                source: `image://ncm/${model.picUrl}`
                                                sourceSize.height: 160
                                                sourceSize.width: 160
                                            }
                                            Label {
                                                Layout.preferredWidth: 160
                                                Layout.topMargin: 8
                                                elide: Text.ElideRight
                                                maximumLineCount: 2
                                                text: model.name
                                                wrapMode: Text.Wrap
                                            }
                                            Label {
                                                Layout.alignment: Qt.AlignHCenter
                                                font.pointSize: Theme.ts.label_small.size
                                                opacity: 0.6
                                                text: Qt.formatDateTime(model.publishTime, 'yyyy')
                                                visible: !!text
                                            }
                                            Item {
                                                Layout.fillHeight: true
                                            }
                                        }

                                        onClicked: {
                                            QA.route(model.itemId);
                                        }
                                    }
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
