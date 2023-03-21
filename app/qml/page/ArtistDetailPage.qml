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
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Material.elevation: 1
                        Material.background: Theme.color.surface_1
                        padding: 0

                        ListView {
                            id: view

                            anchors.fill: parent
                            implicitHeight: contentHeight
                            boundsBehavior: Flickable.StopAtBounds
                            interactive: flick.atYEnd
                            clip: true
                            model: itemData.hotSongs

                            delegate: SongDelegate {
                                width: view.width
                                count: view.count
                                subtitle: `${modelData.album.name}`
                                onClicked: {
                                    QA.playlist.switchTo(modelData);
                                }
                            }

                            footer: ColumnLayout {
                                width: view.width
                                implicitHeight: busy_footer.running ? busy_footer.implicitHeight : 0

                                BusyIndicator {
                                    id: busy_footer

                                    Layout.alignment: Qt.AlignCenter
                                    running: qr_artist.status === ApiQuerierBase.Querying
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

    ApiContainer {
        ArtistQuerier {
            id: qr_artist

            autoReload: root.itemId.valid()
        }

    }

}
