import QtQuick
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Service.Ncm as QNCM
import Qcm.Material as MD

MD.Page {
    id: root
    padding: 0

    ColumnLayout {
        spacing: 0
        anchors.fill: parent

        MD.TabBar {
            id: bar
            Layout.fillWidth: true

            Component.onCompleted: {
                currentIndexChanged();
            }

            MD.TabButton {
                text: qsTr("Song")
            }
            MD.TabButton {
                text: qsTr("Album")
            }
            MD.TabButton {
                text: qsTr("Playlist")
            }
            MD.TabButton {
                text: qsTr("Djradio")
            }

            onCurrentIndexChanged: {}
        }

        MD.Pane {
            Layout.fillHeight: true
            Layout.fillWidth: true
            padding: 0
            backgroundColor: MD.Token.color.surface

            StackLayout {
                id: item_stack
                anchors.fill: parent
                currentIndex: bar.currentIndex

                BaseView {
                    type: QNCM.enums.IdTypeSong
                    delegate: QA.SongDelegate {
                        width: ListView.view.contentWidth
                        dgModel: model.data
                        onClicked: {
                            QA.App.playlist.switchTo(dgModel);
                        }
                    }
                }
                BaseView {
                    type: QNCM.enums.IdTypeAlbum
                    delegate: MD.ListItem {
                        required property var model
                        required property var index

                        width: ListView.view.contentWidth
                        text: model.data.name
                        maximumLineCount: 2
                        supportText: `${QA.Global.join_name(model.data.artists, '/')} - ${Qt.formatDateTime(model.data.publishTime, 'yyyy.M.d')} - ${model.data.trackCount} tracks`
                        leader: MD.Image {
                            radius: 8
                            source: `image://ncm/${model.data.picUrl}`
                            sourceSize.height: 48
                            sourceSize.width: 48
                        }
                        onClicked: {
                            QA.Global.route(model.data.itemId);
                            ListView.view.currentIndex = index;
                        }
                    }
                }
                BaseView {
                    type: QNCM.enums.IdTypePlaylist
                    delegate: MD.ListItem {
                        required property var model
                        required property var index

                        width: ListView.view.contentWidth
                        text: model.data.name
                        maximumLineCount: 2
                        supportText: `${model.data.trackCount} songs`
                        leader: MD.Image {
                            radius: 8
                            source: `image://ncm/${model.data.picUrl}`
                            sourceSize.height: 48
                            sourceSize.width: 48
                        }
                        onClicked: {
                            QA.Global.route(model.data.itemId);
                            ListView.view.currentIndex = index;
                        }
                    }
                }
                BaseView {
                    type: QNCM.enums.IdTypeDjradio
                    delegate: MD.ListItem {
                        required property var model
                        required property var index

                        width: ListView.view.contentWidth
                        text: model.data.name
                        maximumLineCount: 2
                        supportText: `${model.data.programCount} programs`
                        leader: MD.Image {
                            radius: 8
                            source: `image://ncm/${model.data.picUrl}`
                            sourceSize.height: 48
                            sourceSize.width: 48
                        }
                        onClicked: {
                            QA.Global.route(model.data.itemId);
                            ListView.view.currentIndex = index;
                        }
                    }
                }
            }
        }
    }

    component BaseView: MD.ListView {
        model: querier.data
        busy: querier.status === QA.enums.Querying
        leftMargin: 24
        rightMargin: 24
        topMargin: 4
        bottomMargin: 4

        property alias type: querier.type

        QNCM.PlayRecordQuerier {
            id: querier
        }
    }
}
