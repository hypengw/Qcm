import Qcm.App
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Qcm.Material as MD
import ".."
import "../component"
import "../part"
import "../js/util.mjs" as Util

Page {
    id: root
    padding: 0

    MFlickable {
        anchors.fill: parent

        ScrollBar.vertical: ScrollBar {
        }

        ColumnLayout {
            anchors.horizontalCenter: parent.horizontalCenter
            height: implicitHeight
            spacing: 12
            width: parent.width

            ColumnLayout {
                spacing: 4

                Pane {
                    ColumnLayout {
                        anchors.fill: parent

                        Label {
                            font.capitalization: Font.Capitalize
                            font.pointSize: Theme.ts.title_large.size
                            text: qsTr('recommend playlists')
                        }
                    }
                }
                Pane {
                    Layout.fillWidth: true
                    padding: 0

                    SwipeView {
                        id: swipe_playlist

                        readonly property int cellWidth: 180
                        readonly property int column: 2
                        readonly property int itemCount: column * row
                        readonly property int row: Math.max(2, Math.floor(width / cellWidth))

                        anchors.fill: parent
                        clip: true
                        currentIndex: swipe_indicator.currentIndex

                        Repeater {
                            model: Util.array_split(qr_rmd_res.data.dailyPlaylists, swipe_playlist.itemCount)

                            GridView {
                                cellHeight: 250
                                cellWidth: width / swipe_playlist.row
                                clip: true
                                implicitHeight: contentHeight
                                interactive: false
                                model: modelData

                                delegate: Item {
                                    width: GridView.view.cellWidth
                                    height: GridView.view.cellHeight
                                    PicGridDelegate {
                                        anchors.centerIn: parent
                                        width: picWidth
                                        height: Math.min(implicitHeight, parent.height)
                                        image.source: `image://ncm/${modelData.picUrl}`
                                        text: modelData.name

                                        onClicked:  {
                                            QA.route(modelData.itemId)
                                        }
                                    }
                                }
                                footer: ListBusyFooter {
                                    running: qr_rmd_res.status === ApiQuerierBase.Querying
                                    width: GridView.view.width
                                }
                            }
                        }
                    }
                }
                PageIndicator {
                    id: swipe_indicator
                    Layout.alignment: Qt.AlignHCenter
                    count: swipe_playlist.count
                    currentIndex: swipe_playlist.currentIndex
                    interactive: true
                }
            }
            ColumnLayout {
                spacing: 4

                Pane {
                    id: title_pane
                    ColumnLayout {
                        anchors.fill: parent

                        Label {
                            font.capitalization: Font.Capitalize
                            font.pointSize: Theme.ts.title_large.size
                            text: qsTr('recommend songs')
                        }
                    }
                }
                Pane {
                    id: tool_pane
                    RowLayout {
                        anchors.fill: parent

                        MD.Button {
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
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    padding: 0

                    ColumnLayout {
                        id: pane_view_column
                        anchors.fill: parent
                        spacing: 0

                        Pane {
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            padding: 0

                            ListView {
                                id: view
                                anchors.fill: parent
                                boundsBehavior: Flickable.StopAtBounds
                                clip: true
                                implicitHeight: contentHeight
                                interactive: false
                                model: qr_rmd_songs.data.dailySongs

                                delegate: SongDelegate {
                                    count: view.count
                                    width: view.width

                                    onClicked: {
                                        QA.playlist.switchTo(modelData);
                                    }
                                }
                                footer: ListBusyFooter {
                                    running: qr_rmd_songs.status === ApiQuerierBase.Querying
                                    width: ListView.view.width
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

            Component.onCompleted: {
                root.visibleChanged.connect(refreshSlot);
                dirtyChanged.connect(refreshSlot);
            }
            onTriggered: dirty = true
        }
    }

    MD.FAB {
        action: Action {
            icon.name: Theme.ic.play_arrow

            onTriggered: {
                const songs = qr_rmd_songs.data.dailySongs.filter(s => {
                        return s.canPlay;
                    });
                if (songs.length)
                    QA.playlist.switchList(songs);
            }
        }
    }
}
