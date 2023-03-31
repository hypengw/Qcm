import QcmApp
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import ".."
import "../component"
import "../part"
import "../js/util.mjs" as Util

Page {
    id: root

    padding: 0

    MFlickable {
        anchors.fill: parent

        ColumnLayout {
            height: implicitHeight
            width: Math.min(800, parent.width)
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 12

            ColumnLayout {
                spacing: 4

                Pane {
                    ColumnLayout {
                        anchors.fill: parent

                        Label {
                            text: qsTr('recommend playlists')
                            font.pointSize: Theme.ts.title_large.size
                            font.capitalization: Font.Capitalize
                        }

                    }

                }

                Pane {
                    padding: 0
                    Material.elevation: 1
                    Material.background: Theme.color.surface_1
                    Layout.fillWidth: true

                    SwipeView {
                        id: swipe_playlist

                        readonly property int cellWidth: 180
                        readonly property int column: 2
                        readonly property int row: Math.max(2, Math.floor(width / cellWidth))
                        readonly property int itemCount: column * row

                        anchors.fill: parent
                        clip: true
                        currentIndex: swipe_indicator.currentIndex

                        Repeater {
                            model: Util.array_split(qr_rmd_res.data.dailyPlaylists, swipe_playlist.itemCount)

                            GridView {
                                clip: true
                                interactive: false
                                model: modelData
                                implicitHeight: contentHeight
                                cellHeight: 250
                                cellWidth: width / swipe_playlist.row

                                delegate: PicGridDelegate {
                                    text: modelData.name
                                    // subText:
                                    image.source: `image://ncm/${modelData.picUrl}`
                                    onClicked: QA.route(modelData.itemId)
                                }

                                footer: ListBusyFooter {
                                    width: GridView.view.width
                                    running: qr_rmd_res.status === ApiQuerierBase.Querying
                                }

                            }

                        }

                    }

                }

                PageIndicator {
                    id: swipe_indicator

                    Layout.alignment: Qt.AlignHCenter
                    interactive: true
                    count: swipe_playlist.count
                    currentIndex: swipe_playlist.currentIndex
                }

            }

            ColumnLayout {
                spacing: 4

                Pane {
                    id: title_pane

                    ColumnLayout {
                        anchors.fill: parent

                        Label {
                            text: qsTr('recommend songs')
                            font.pointSize: Theme.ts.title_large.size
                            font.capitalization: Font.Capitalize
                        }

                    }

                }

                Pane {
                    id: tool_pane

                    RowLayout {
                        anchors.fill: parent

                        MButton {
                            highlighted: true
                            font.capitalization: Font.Capitalize

                            action: Action {
                                icon.name: Theme.ic.play_arrow
                                text: qsTr('play all')
                                onTriggered: {
                                    const songs = qr_rmd_songs.data.dailySongs.filter((s) => {
                                        return s.canPlay;
                                    });
                                    if (songs.length)
                                        QA.playlist.switchList(songs);

                                }
                            }

                        }

                        MButton {
                            highlighted: true
                            Material.accent: Theme.color.secondary
                            font.capitalization: Font.Capitalize

                            action: Action {
                                icon.name: Theme.ic.playlist_add
                                text: qsTr('add to list')
                                onTriggered: {
                                    QA.playlist.appendList(qr_rmd_songs.data.dailySongs);
                                }
                            }

                        }

                    }

                }

                Pane {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
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
                                interactive: false
                                clip: true
                                model: qr_rmd_songs.data.dailySongs

                                delegate: SongDelegate {
                                    width: view.width
                                    count: view.count
                                    onClicked: {
                                        QA.playlist.switchTo(modelData);
                                    }
                                }

                                footer: ListBusyFooter {
                                    width: ListView.view.width
                                    running: qr_rmd_songs.status === ApiQuerierBase.Querying
                                }

                            }

                        }

                    }

                }

            }

        }

        ApiContainer {
            RecommendSongsQuerier {
                id: qr_rmd_songs
            }

            RecommendResourceQuerier {
                id: qr_rmd_res
            }

        }

        Timer {
            id: timer_refresh

            property bool dirty: false

            function refreshSlot() {
                if (root.visible && dirty) {
                    qr_rmd_res.query();
                    qr_rmd_songs.query();
                    dirty = false;
                }
            }

            interval: 15 * 60 * 1000
            repeat: true
            running: true
            onTriggered: dirty = true
            Component.onCompleted: {
                root.visibleChanged.connect(refreshSlot);
                dirtyChanged.connect(refreshSlot);
            }
        }

        ScrollBar.vertical: ScrollBar {
        }

    }

}
