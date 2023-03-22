import QcmApp
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import ".."
import "../component"
import "../part"

Page {
    id: root

    padding: 16

    ColumnLayout {
        width: Math.min(800, parent.width)
        height: parent.height
        anchors.horizontalCenter: parent.horizontalCenter
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
                            const songs = qr_rmd.data.dailySongs.filter((s) => {
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
                            QA.playlist.appendList(qr_rmd.data.dailySongs);
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
                        interactive: true
                        clip: true
                        model: qr_rmd.data.dailySongs

                        delegate: SongDelegate {
                            width: view.width
                            count: view.count
                            onClicked: {
                                QA.playlist.switchTo(modelData);
                            }
                        }

                        footer: ListBusyFooter {
                            width: ListView.view.width
                            running: qr_rmd.status === ApiQuerierBase.Querying
                        }

                        ScrollBar.vertical: ScrollBar {
                        }

                    }

                }

            }

        }

    }

    ApiContainer {
        RecommandSongsQuerier {
            id: qr_rmd
        }

    }

}
