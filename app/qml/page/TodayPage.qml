import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD
import "../js/util.mjs" as Util

MD.Page {
    id: root
    padding: 0

    MD.Flickable {
        anchors.fill: parent

        ColumnLayout {
            anchors.horizontalCenter: parent.horizontalCenter
            height: implicitHeight
            spacing: 12
            width: parent.width

            ColumnLayout {
                spacing: 4

                MD.Pane {
                    ColumnLayout {
                        anchors.fill: parent

                        MD.Text {
                            font.capitalization: Font.Capitalize
                            typescale: MD.Token.typescale.headline_large
                            text: qsTr('recommend playlists')
                        }
                    }
                }
                MD.Pane {
                    Layout.fillWidth: true
                    MD.MatProp.backgroundColor: MD.Token.color.surface
                    padding: 0

                    SwipeView {
                        id: swipe_playlist

                        readonly property int fixedCellWidth: Math.max(160, QA.Global.main_win.width / 6.0)
                        readonly property int column: 2
                        readonly property int itemCount: column * row
                        readonly property int row: Math.max(2, Math.floor(width / fixedCellWidth))

                        anchors.fill: parent
                        clip: true
                        currentIndex: swipe_indicator.currentIndex

                        Repeater {
                            model: Util.array_split(qr_rmd_res.data.dailyPlaylists, swipe_playlist.itemCount)

                            QA.MGridView {
                                fixedCellWidth: swipe_playlist.fixedCellWidth
                                clip: true
                                implicitHeight: contentHeight
                                interactive: false
                                model: modelData

                                delegate: Item {
                                    width: GridView.view.cellWidth
                                    height: GridView.view.cellHeight
                                    QA.PicGridDelegate {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        anchors.top: parent.top
                                        anchors.topMargin: 12

                                        picWidth: parent.GridView.view.fixedCellWidth
                                        width: picWidth
                                        height: Math.min(implicitHeight, parent.height)
                                        image.source: `image://ncm/${modelData.picUrl}`
                                        text: modelData.name

                                        onClicked: {
                                            QA.Global.route(modelData.itemId);
                                        }
                                    }
                                }
                                footer: MD.ListBusyFooter {
                                    running: qr_rmd_res.status === QA.ApiQuerierBase.Querying
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

                MD.Pane {
                    id: title_pane
                    ColumnLayout {
                        anchors.fill: parent

                        MD.Text {
                            font.capitalization: Font.Capitalize
                            typescale: MD.Token.typescale.headline_large
                            text: qsTr('recommend songs')
                        }
                    }
                }
                MD.Pane {
                    id: tool_pane
                    RowLayout {
                        anchors.fill: parent

                        MD.Button {
                            font.capitalization: Font.Capitalize
                            action: Action {
                                icon.name: MD.Token.icon.playlist_add
                                text: qsTr('add to list')

                                onTriggered: {
                                    QA.Global.playlist.appendList(qr_rmd_songs.data.dailySongs);
                                }
                            }
                        }
                    }
                }
                MD.Pane {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    padding: 0

                    ColumnLayout {
                        id: pane_view_column
                        anchors.fill: parent
                        spacing: 0

                        MD.Pane {
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            padding: 0

                        MD.ListView {
                                id: view
                                anchors.fill: parent
                                clip: true
                                implicitHeight: contentHeight
                                interactive: false
                                model: qr_rmd_songs.data.dailySongs

                                delegate: QA.SongDelegate {
                                    count: view.count
                                    width: view.width

                                    onClicked: {
                                        QA.Global.playlist.switchTo(modelData);
                                    }
                                }
                                footer: MD.ListBusyFooter {
                                    running: qr_rmd_songs.status === QA.ApiQuerierBase.Querying
                                    width: ListView.view.width
                                }
                            }
                        }
                    }
                }
            }
        }
        QA.RecommendSongsQuerier {
            id: qr_rmd_songs
        }
        QA.RecommendResourceQuerier {
            id: qr_rmd_res
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
            icon.name: MD.Token.icon.play_arrow

            onTriggered: {
                const songs = qr_rmd_songs.data.dailySongs.filter(s => {
                        return s.canPlay;
                    });
                if (songs.length)
                    QA.Global.playlist.switchList(songs);
            }
        }
    }
}
