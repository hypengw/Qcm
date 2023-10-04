import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root

    property alias itemData: qr_al.data
    property alias itemId: qr_al.itemId

    padding: 0

    ListView {
        id: view
        anchors.fill: parent
        boundsBehavior: Flickable.StopAtBounds
        reuseItems: true
        clip: true
        contentY: 0

        topMargin: 8

        model: itemData.songs
        header: ColumnLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottomMargin: 8
            anchors.leftMargin: 8
            anchors.rightMargin: 8

            spacing: 4

            RowLayout {
                spacing: 16

                MD.Image {
                    MD.MatProp.elevation: MD.Token.elevation.level2
                    source: `image://ncm/${root.itemData.picUrl}`
                    sourceSize.height: 240
                    sourceSize.width: 240
                    radius: 16
                }

                ColumnLayout {
                    Layout.alignment: Qt.AlignTop
                    spacing: 12

                    MD.Text {
                        Layout.fillWidth: true
                        maximumLineCount: 2
                        text: root.itemData.name
                        typescale: MD.Token.typescale.headline_large
                    }
                    RowLayout {
                        spacing: 12
                        MD.Text {
                            typescale: MD.Token.typescale.body_medium
                            text: `${root.itemData.size} tracks`
                        }
                        MD.Text {
                            typescale: MD.Token.typescale.body_medium
                            text: Qt.formatDateTime(root.itemData.publishTime, 'yyyy.MM')
                        }
                    }
                    MD.Text {
                        typescale: MD.Token.typescale.body_medium
                        text: QA.Global.join_name(root.itemData.artists, '/')
                        /*
                        onClicked: {
                            const artists = root.itemData.artists;
                            if (artists.length === 1)
                                QA.Global.route(artists[0].itemId);
                            else
                                QA.Global.show_popup('qrc:/Qcm/App/qml/part/ArtistsPopup.qml', {
                                        "model": artists
                                    });
                        }
                        */
                    }

                    QA.ListDescription {
                        description: root.itemData.description.trim()
                        Layout.fillWidth: true
                    }
                }
            }
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                MD.IconButton {
                    action: Action {
                        icon.name: MD.Token.icon.playlist_add
                        // text: qsTr('add to list')
                        onTriggered: {
                            QA.playlist.appendList(itemData.songs);
                        }
                    }
                }
                MD.IconButton {
                    id: btn_fav
                    property bool liked: qr_dynamic.data.isSub
                    action: Action {
                        icon.name: btn_fav.liked ? MD.Token.icon.done : MD.Token.icon.add
                        onTriggered: {
                            qr_sub.sub = !btn_fav.liked;
                            qr_sub.itemId = root.itemId;
                            qr_sub.query();
                        }
                    }
                    Binding on liked  {
                        value: qr_sub.sub
                        when: qr_sub.status === QA.ApiQuerierBase.Finished
                    }
                }
            }
        }
        delegate: QA.SongDelegate {
            count: view.count
            width: view.width
            subtitle: QA.Global.join_name(modelData.artists, '/')

            onClicked: {
                QA.Global.playlist.switchTo(modelData);
            }
        }
        footer: QA.ListBusyFooter {
            running: qr_al.status === QA.ApiQuerierBase.Querying
            width: ListView.view.width
        }
        ScrollBar.vertical: ScrollBar {
        }
    }
    MD.FAB {
        action: Action {
            icon.name: MD.Token.icon.play_arrow
            onTriggered: {
                const songs = itemData.songs.filter(s => {
                        return s.canPlay;
                    });
                if (songs.length)
                    QA.Global.playlist.switchList(songs);
            }
        }
    }

    QA.AlbumDetailQuerier {
        id: qr_al
        autoReload: root.itemId.valid()
    }
    QA.AlbumDetailDynamicQuerier {
        id: qr_dynamic
        autoReload: itemId.valid()
        itemId: qr_al.itemId
    }
    QA.AlbumSubQuerier {
        id: qr_sub
        autoReload: false

        onStatusChanged: {
            if (status === QA.ApiQuerierBase.Finished)
                QA.Global.sig_like_album();
        }
    }
}
