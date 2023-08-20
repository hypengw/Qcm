import Qcm.App
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import ".."
import "../component"
import "../part"

Page {
    id: root

    property alias itemData: qr_al.data
    property alias itemId: qr_al.itemId

    padding: 0

    MFlickable {
        id: flick
        anchors.fill: parent
        clip: true

        ScrollBar.vertical: ScrollBar {
        }

        ColumnLayout {
            id: content
            anchors.horizontalCenter: parent.horizontalCenter
            height: implicitHeight
            spacing: 4
            width: Math.min(800, parent.width)

            Pane {
                Layout.fillWidth: true
                padding: 0

                ColumnLayout {
                    anchors.fill: parent

                    Pane {
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        RowLayout {
                            anchors.fill: parent
                            spacing: 16

                            Pane {
                                Layout.alignment: Qt.AlignTop
                                Layout.preferredHeight: Layout.preferredWidth
                                Layout.preferredWidth: 160 + 2 * padding
                                Material.background: Theme.color.surface_2
                                Material.elevation: 4
                                padding: 4

                                Image {
                                    height: width
                                    source: `image://ncm/${root.itemData.picUrl}`
                                    sourceSize.height: 160
                                    sourceSize.width: 160
                                    width: 160
                                }
                            }
                            ColumnLayout {
                                Layout.alignment: Qt.AlignTop
                                spacing: 0

                                Label {
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                    font.bold: true
                                    font.pointSize: Theme.ts.title_medium.size
                                    maximumLineCount: 2
                                    text: root.itemData.name
                                    wrapMode: Text.Wrap
                                }
                                InfoRow {
                                    icon_name: Theme.ic.album
                                    label_text: `${root.itemData.size} tracks`
                                }
                                MButton {
                                    Layout.fillWidth: true
                                    bottomInset: 0
                                    flat: true
                                    font.pointSize: Theme.ts.label_medium.size
                                    topInset: 0
                                    verticalPadding: 0

                                    contentItem: IconRowLayout {
                                        iconSize: 16
                                        text: Theme.ic.person

                                        Label {
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                            text: QA.join_name(root.itemData.artists, '/')
                                        }
                                    }

                                    onClicked: {
                                        const artists = root.itemData.artists;
                                        if (artists.length === 1)
                                            QA.route(artists[0].itemId);
                                        else
                                            QA.show_popup('qrc:/Qcm/App/qml/part/ArtistsPopup.qml', {
                                                    "model": artists
                                                });
                                    }
                                }
                                InfoRow {
                                    icon_name: Theme.ic.today
                                    label_text: Qt.formatDateTime(root.itemData.publishTime, 'yyyy')
                                }
                                Item {
                                    Layout.fillHeight: true
                                }
                                MButton {
                                    id: btn_desc

                                    readonly property string description: root.itemData.description.trim()

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
                                        QA.show_page_popup('qrc:/Qcm/App/qml/page/DescriptionPage.qml', {
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
                RowLayout {
                    anchors.fill: parent

                    MButton {
                        font.capitalization: Font.Capitalize
                        highlighted: true

                        action: Action {
                            icon.name: Theme.ic.play_arrow
                            text: qsTr('play all')

                            onTriggered: {
                                const songs = itemData.songs.filter(s => {
                                        return s.canPlay;
                                    });
                                if (songs.length)
                                    QA.playlist.switchList(songs);
                            }
                        }
                    }
                    MButton {
                        Material.accent: Theme.color.secondary
                        font.capitalization: Font.Capitalize
                        highlighted: true

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

                        Material.accent: Theme.color.secondary
                        font.capitalization: Font.Capitalize
                        highlighted: true

                        action: Action {
                            icon.name: btn_fav.liked ? Theme.ic.done : Theme.ic.add
                            text: qsTr(btn_fav.liked ? 'fav-ed' : 'fav')

                            onTriggered: {
                                qr_sub.sub = !btn_fav.liked;
                                qr_sub.itemId = root.itemId;
                                qr_sub.query();
                            }
                        }
                        Binding on liked  {
                            value: qr_sub.sub
                            when: qr_sub.status === ApiQuerierBase.Finished
                        }
                    }
                }
            }
            Pane {
                Layout.fillWidth: true
                padding: 0

                ColumnLayout {
                    id: pane_view_column
                    anchors.fill: parent
                    spacing: 0

                    Pane {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        // Material.elevation: 1
                        padding: 0

                        MListView {
                            id: view
                            anchors.fill: parent
                            boundsBehavior: Flickable.StopAtBounds
                            clip: true
                            implicitHeight: contentHeight
                            interactive: flick.atYEnd
                            model: itemData.songs
                            reuseItems: true

                            delegate: SongDelegate {
                                count: view.count
                                subtitle: QA.join_name(modelData.artists, '/')
                                width: view.width

                                onClicked: {
                                    QA.playlist.switchTo(modelData);
                                }
                            }
                            footer: ListBusyFooter {
                                running: qr_al.status === ApiQuerierBase.Querying
                                width: ListView.view.width
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
            autoReload: itemId.valid()
            itemId: qr_al.itemId
        }
        AlbumSubQuerier {
            id: qr_sub
            autoReload: false

            onStatusChanged: {
                if (status === ApiQuerierBase.Finished)
                    QA.sig_like_album();
            }
        }
    }
}
