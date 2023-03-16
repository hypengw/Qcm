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
                    font.pointSize: 18
                    font.capitalization: Font.Capitalize
                }

            }

        }

        Pane {
            id: tool_pane

            RowLayout {
                anchors.fill: parent

                Button {
                    highlighted: true
                    Material.foreground: Theme.color.on_primary
                    onClicked: {
                        QA.playlist.switchList(qr_rmd.data.dailySongs);
                    }

                    contentItem: IconRowLayout {
                        text: Theme.ic.play_arrow
                        iconSize: 16

                        Label {
                            text: qsTr('play all')
                            font.pointSize: 12
                            font.capitalization: Font.Capitalize
                        }

                    }

                }

                Button {
                    highlighted: true
                    Material.foreground: Theme.color.on_secondary
                    Material.accent: Theme.color.secondary
                    onClicked: {
                        QA.playlist.appendList(qr_rmd.data.dailySongs);
                    }

                    contentItem: IconRowLayout {
                        text: Theme.ic.playlist_add
                        iconSize: 16

                        Label {
                            text: qsTr('add to list')
                            font.pointSize: 12
                            font.capitalization: Font.Capitalize
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
                        interactive: flick.atYEnd
                        clip: true
                        model: qr_rmd.data.dailySongs

                        delegate: SongDelegate {
                            width: view.width
                            count: view.count
                            onClicked: {
                                QA.playlist.switchTo(modelData);
                            }
                        }

                        ScrollBar.vertical: ScrollBar {
                        }

                    }

                }

            }

        }

    }

    RecommandSongsQuerier {
        id: qr_rmd
    }

}
