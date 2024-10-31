import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs

import Qcm.App as QA
import Qcm.Service.Ncm as QNCM
import Qcm.Material as MD

MD.Page {
    id: root
    padding: 0
    title: qsTr("cloud")
    property int vpadding: header.visible ? 0 : root.MD.MatProp.size.verticalPadding + m_view_pane.topMargin

    Item {
        anchors.fill: parent
        implicitWidth: m_view.implicitWidth
        implicitHeight: m_view.implicitHeight
        clip: true

        MD.FlickablePane {
            id: m_view_pane
            view: m_view
            visible: !root.background.visible
            color: root.backgroundColor
            corners: MD.Util.corner(root.header.visible ? 0 : root.radius, root.radius)
        }

        MD.ListView {
            id: m_view
            anchors.fill: parent
            expand: true
            topMargin: root.vpadding
            bottomMargin: root.vpadding

            leftMargin: 24
            rightMargin: 24

            header: Item {
                width: parent.width
                implicitHeight: children[0].implicitHeight
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0
                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter

                        MD.IconButton {
                            action: QA.AppendListAction {
                                getSongs: function () {
                                    const songs = [];
                                    const model = qr_cloud.data;
                                    for (let i = 0; i < model.rowCount(); i++) {
                                        songs.push(model.item(i).song);
                                    }
                                    return songs;
                                }
                            }
                        }
                        MD.IconButton {
                            action: Action {
                                icon.name: MD.Token.icon.upload
                                onTriggered: {
                                    m_file_dialog.open();
                                }
                            }
                        }
                    }
                }
            }
            model: qr_cloud.data
            delegate: QA.SongDelegate {
                width: ListView.view.contentWidth
                dgModel: model.song

                onClicked: {
                    QA.Action.play_by_id(dgModel.itemId);
                }
            }

            QNCM.UserCloudQuerier {
                id: qr_cloud
            }
            Timer {
                id: timer_refresh

                property bool dirty: false

                function refreshSlot() {
                    if (root.visible && dirty) {
                        if (qr_cloud.offset === 0)
                            qr_cloud.query();
                        else
                            qr_cloud.offset = 0;
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
            MD.FAB {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: 16
                anchors.bottomMargin: 16
                flickable: m_view
                action: Action {
                    icon.name: MD.Token.icon.play_arrow

                    onTriggered: {
                        const songs = [];
                        const model = qr_cloud.data;
                        for (let i = 0; i < model.rowCount(); i++) {
                            songs.push(model.item(i).song);
                        }
                        if (songs.length)
                            QA.App.playqueue.switchList(songs);
                    }
                }
            }
        }

        FileDialog {
            id: m_file_dialog
            onAccepted: {
                m_upload_api.upload(currentFile);
            }
        }

        QNCM.CloudUploadApi {
            id: m_upload_api
            onStatusChanged: {
                if (status === QA.enums.Finished) {
                    qr_cloud.reset();
                }
            }
        }
    }
}
