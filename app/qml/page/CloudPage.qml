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

    MD.ListView {
        id: m_view
        anchors.fill: parent

        header: Item {
            width: parent.width
            implicitHeight: children[0].implicitHeight
            ColumnLayout {
                anchors.fill: parent
                anchors.bottomMargin: 8
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 4

                MD.Pane {
                    id: title_pane
                    ColumnLayout {
                        anchors.fill: parent

                        MD.Text {
                            font.capitalization: Font.Capitalize
                            text: qsTr('cloud songs')
                            typescale: MD.Token.typescale.headline_large
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
                                const model = qr_cloud.data;
                                for (let i = 0; i < model.rowCount(); i++) {
                                    songs.push(model.item(i).song);
                                }
                                if (songs.length)
                                    QA.Global.playlist.appendList(songs);
                            }
                        }
                    }
                }
            }
        }
        model: qr_cloud.data
        delegate: QA.SongDelegate {
            width: ListView.view.width
            model_: model.song

            onClicked: {
                QA.Global.playlist.switchTo(model_);
            }
        }

        QNcm.UserCloudQuerier {
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
    }

    MD.FAB {
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
                    QA.Global.playlist.switchList(songs);
            }
        }
    }
}
