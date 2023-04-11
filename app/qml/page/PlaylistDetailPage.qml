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

    property alias itemData: qr_pl.data
    property alias itemId: qr_pl.itemId

    padding: 0

    MFlickable {
        id: flick
        anchors.fill: parent
        clip: true
        contentHeight: content.implicitHeight

        ScrollBar.vertical: ScrollBar {
            visible: false
        }

        ColumnLayout {
            id: content
            anchors.horizontalCenter: parent.horizontalCenter
            height: parent.height
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
                                    source: `image://ncm/${root.itemData.picUrl}`
                                    sourceSize.height: 160
                                    sourceSize.width: 160
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
                                    text: root.itemData.name
                                    wrapMode: Text.Wrap
                                }
                                InfoRow {
                                    icon_name: Theme.ic.music_note
                                    label_text: `${root.itemData.songs.length} songs`
                                }
                                InfoRow {
                                    icon_name: Theme.ic.today
                                    label_text: Qt.formatDateTime(root.itemData.updateTime, 'yyyy.MM.dd')
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

                        property bool liked: qr_dynamic.data.subscribed

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
                implicitHeight: Math.min(root.height * 0.75, pane_view_column.implicitHeight)
                padding: 0

                ColumnLayout {
                    id: pane_view_column
                    anchors.fill: parent
                    spacing: 0

                    Pane {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
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

                            ScrollBar.vertical: ScrollBar {
                            }
                            delegate: SongDelegate {
                                count: view.count
                                width: view.width

                                onClicked: {
                                    QA.playlist.switchTo(modelData);
                                }
                            }
                            footer: ListBusyFooter {
                                running: qr_pl.status === ApiQuerierBase.Querying
                                width: ListView.view.width
                            }
                        }
                    }
                }
            }
        }
    }
    ApiContainer {
        PlaylistDetailQuerier {
            id: qr_pl
            autoReload: root.itemId.valid()
        }
        PlaylistDetailDynamicQuerier {
            id: qr_dynamic
            autoReload: itemId.valid()
            itemId: qr_pl.itemId
        }
        PlaylistSubscribeQuerier {
            id: qr_sub
            autoReload: false

            onStatusChanged: {
                if (status === ApiQuerierBase.Finished)
                    QA.sig_like_playlist();
            }
        }
    }
}
