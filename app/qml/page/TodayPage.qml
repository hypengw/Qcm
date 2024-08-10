import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD
import "../js/util.mjs" as Util

MD.Page {
    id: root
    padding: 0

    MD.Flickable {
        id: m_fk
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
                            text: qsTr('recommended playlists')
                        }
                    }
                }
                MD.Pane {
                    Layout.fillWidth: true
                    padding: 0

                    SwipeView {
                        id: swipe_playlist

                        readonly property int fixedCellWidth: Math.max(160, QA.Global.main_win.width / 6.0)
                        readonly property int column: 2
                        readonly property int space: 8
                        readonly property int row: Math.max(2, Math.floor((width - space) / (fixedCellWidth + space / 2)))
                        readonly property int itemCount: column * row

                        anchors.fill: parent
                        currentIndex: swipe_indicator.currentIndex

                        Repeater {
                            model: Util.array_split(qr_rmd_res.data.dailyPlaylists, swipe_playlist.itemCount)

                            QA.MGridView {
                                fixedCellWidth: swipe_playlist.fixedCellWidth
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
                                    running: qr_rmd_res.status === QA.enums.Querying
                                    width: GridView.view.width
                                }
                            }
                        }
                    }
                }
                MD.PageIndicator {
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
                            text: qsTr('recommended songs')
                        }
                    }
                }
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    MD.IconButton {
                        action: QA.AppendListAction {
                            getSongs: function () {
                                return qr_rmd_songs.data.dailySongs;
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
                                implicitHeight: contentHeight
                                interactive: false
                                topMargin: 8
                                bottomMargin: 8
                                leftMargin: 24
                                rightMargin: 24


                                model: qr_rmd_songs.data.dailySongs

                                delegate: QA.SongDelegate {
                                    width: ListView.view.contentWidth
                                    showCover: true
                                    onClicked: {
                                        QA.Global.playlist.switchTo(modelData);
                                    }
                                }
                                footer: MD.ListBusyFooter {
                                    running: qr_rmd_songs.status === QA.enums.Querying
                                    width: ListView.view.contentWidth
                                }
                            }
                        }
                    }
                }
            }
        }
        QNcm.RecommendSongsQuerier {
            id: qr_rmd_songs
        }
        QNcm.RecommendResourceQuerier {
            id: qr_rmd_res
        }
        // avoid loading with switch page
        Timer {
            id: timer_refresh_delay

            property bool dirty: false
            interval: 3 * 1000
            repeat: false
            running: false
            onTriggered: {
                if (root.visible && dirty) {
                    qr_rmd_res.query();
                    qr_rmd_songs.query();
                    dirty = false;
                }
            }
            
        }
        Connections {
            target: root
            function onVisibleChanged() { 
                timer_refresh_delay.start();
            }
        }
        Timer {
            id: timer_refresh

            interval: 15 * 60 * 1000
            repeat: true
            running: true

            onTriggered: {
                timer_refresh_delay.dirty = true
                timer_refresh_delay.start();
            }
        }
    }

    MD.FAB {
        flickable: m_fk
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
