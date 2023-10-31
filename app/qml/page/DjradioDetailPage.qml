import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root

    property alias itemData: qr_dj.data
    property alias itemId: qr_dj.itemId

    padding: 0

    MD.ListView {
        id: view
        anchors.fill: parent
        reuseItems: true
        clip: true
        contentY: 0

        topMargin: 8

        model: qr_program.data

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
                            text: `${root.itemData.programCount} programs`
                        }
                        MD.Text {
                            typescale: MD.Token.typescale.body_medium
                            text: Qt.formatDateTime(root.itemData.createTime, 'yyyy.MM.dd')
                        }
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
                            const songs = [];
                            const model = qr_program.data;
                            for (let i = 0; i < model.rowCount(); i++) {
                                songs.push(model.item(i).song);
                            }
                            QA.playlist.appendList(songs);
                        }
                    }
                }
                MD.IconButton {
                    id: btn_fav
                    property bool liked: root.itemData.subed
                    action: Action {
                        icon.name: btn_fav.liked ? MD.Token.icon.done : MD.Token.icon.add

                        // text: qsTr(btn_fav.liked ? 'fav-ed' : 'fav')

                        onTriggered: {
                            qr_sub.sub = !btn_fav.liked;
                            qr_sub.itemId = root.itemId;
                            qr_sub.query();
                        }
                    }
                    Binding on liked {
                        value: qr_sub.sub
                        when: qr_sub.status === QA.ApiQuerierBase.Finished
                    }
                }
                MD.IconButton {
                    id: btn_comment
                    action: Action {
                        icon.name: MD.Token.icon.comment
                        onTriggered: {
                            QA.Global.show_page_popup('qrc:/Qcm/App/qml/page/CommentPage.qml', {
                                    "itemId": root.itemId
                                });
                        }
                    }
                }
            }
        }
        delegate: QA.ProgramDelegate {
            count: view.count
            width: view.width

            model_: QA.App.program(model)

            onClicked: {
                QA.Global.playlist.switchTo(model.song);
            }
        }
        footer: MD.ListBusyFooter {
            running: qr_dj.status === QA.ApiQuerierBase.Querying
            width: ListView.view.width
        }
    }
    MD.FAB {
        action: Action {
            icon.name: MD.Token.icon.play_arrow

            onTriggered: {
                const songs = [];
                const model = qr_program.data;
                for (let i = 0; i < model.rowCount(); i++) {
                    songs.push(model.item(i).song);
                }
                if (songs.length)
                    QA.Global.playlist.switchList(songs);
            }
        }
    }

    QA.DjradioDetailQuerier {
        id: qr_dj
        autoReload: root.itemId.valid()
    }
    QA.DjradioProgramQuerier {
        id: qr_program
        autoReload: itemId.valid()
        itemId: qr_dj.itemId
    }
    QA.DjradioSubQuerier {
        id: qr_sub
        autoReload: false

        onStatusChanged: {
            if (status === QA.ApiQuerierBase.Finished) {
                QA.App.djradioLiked(itemId, sub);
            }
        }
    }
}
