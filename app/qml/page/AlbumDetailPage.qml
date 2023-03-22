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

    property alias itemId: qr_al.itemId
    property alias itemData: qr_al.data

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
        clip: true
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

                            Pane {
                                Layout.alignment: Qt.AlignTop
                                Layout.preferredWidth: 160 + 2 * padding
                                Layout.preferredHeight: Layout.preferredWidth
                                Material.elevation: 4
                                Material.background: Theme.color.surface_2
                                padding: 4

                                Image {
                                    width: 160
                                    height: width
                                    source: `image://ncm/${root.itemData.picUrl}`
                                    sourceSize.width: 160
                                    sourceSize.height: 160
                                }

                            }

                            ColumnLayout {
                                Layout.alignment: Qt.AlignTop
                                spacing: 12

                                Label {
                                    Layout.fillWidth: true
                                    text: root.itemData.name
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
                                        text: `${root.itemData.size} tracks`
                                    }

                                }

                                IconRowLayout {
                                    text: Theme.ic.person
                                    iconSize: 16

                                    Label {
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                        text: QA.join_name(root.itemData.artists, '/')
                                    }

                                }

                                IconRowLayout {
                                    text: Theme.ic.today
                                    iconSize: 16

                                    Label {
                                        Layout.fillWidth: true
                                        elide: Text.ElideRight
                                        text: Qt.formatDateTime(root.itemData.publishTime, 'yyyy')
                                    }

                                }

                                Item {
                                    Layout.fillHeight: true
                                }

                                IconRowLayout {
                                    Layout.alignment: Qt.AlignBottom
                                    iconSize: 16
                                    text: Theme.ic.info
                                    visible: root.itemData.description.trim()

                                    Label {
                                        Layout.fillWidth: true
                                        maximumLineCount: 2
                                        wrapMode: Text.Wrap
                                        elide: Text.ElideRight
                                        textFormat: Text.PlainText
                                        text: `${root.itemData.description}`.trim()
                                    }

                                }

                            }

                        }

                    }

                }

            }

            Pane {
                RowLayout {
                    anchors.fill: parent

                    MButton {
                        highlighted: true
                        font.capitalization: Font.Capitalize

                        action: Action {
                            icon.name: Theme.ic.play_arrow
                            text: qsTr('play all')
                            onTriggered: {
                                const songs = itemData.songs.filter((s) => {
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
                                QA.playlist.appendList(itemData.songs);
                            }
                        }

                    }

                    MButton {
                        id: btn_fav

                        property bool liked: qr_dynamic.data.isSub

                        highlighted: true
                        Material.accent: Theme.color.secondary
                        font.capitalization: Font.Capitalize

                        Binding on liked {
                            when: qr_sub.status === ApiQuerierBase.Finished
                            value: qr_sub.sub
                        }

                        action: Action {
                            icon.name: btn_fav.liked ? Theme.ic.done : Theme.ic.add
                            text: qsTr(btn_fav.liked ? 'fav-ed' : 'fav')
                            onTriggered: {
                                qr_sub.sub = !btn_fav.liked;
                                qr_sub.itemId = root.itemId;
                                qr_sub.query();
                            }
                        }

                    }

                }

            }

            Pane {
                Layout.fillWidth: true
                padding: 0
                implicitHeight: Math.min(root.height * 0.75, pane_view_column.implicitHeight)

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
                            model: itemData.songs

                            delegate: SongDelegate {
                                width: view.width
                                count: view.count
                                subtitle: QA.join_name(modelData.artists, '/')
                                onClicked: {
                                    QA.playlist.switchTo(modelData);
                                }
                            }

                            footer: ListBusyFooter {
                                width: ListView.view.width
                                running: qr_al.status === ApiQuerierBase.Querying
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
        AlbumDetailQuerier {
            id: qr_al

            autoReload: root.itemId.valid()
        }

        AlbumDetailDynamicQuerier {
            id: qr_dynamic

            itemId: qr_al.itemId
            autoReload: itemId.valid()
        }

        AlbumSubQuerier {
            id: qr_sub

            autoReload: false
        }

    }

}
