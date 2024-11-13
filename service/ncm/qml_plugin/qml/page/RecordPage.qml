import QtQuick
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Service.Ncm as QNCM
import Qcm.Material as MD

MD.Page {
    id: root
    title: qsTr('history')
    padding: 0
    topPadding: MD.MatProp.size.verticalPadding

    ColumnLayout {
        spacing: 0
        anchors.fill: parent

        MD.TabBar {
            id: m_bar
            Layout.fillWidth: true
            corners: MD.Util.corner(root.header.visible ? 0 : root.radius, 0)

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

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true

            MD.FlickablePane {
                id: m_view_pane
                view: item_stack.children[item_stack.currentIndex]
                corners: MD.Util.corner(0, root.radius)
                color: root.MD.MatProp.backgroundColor
            }

            StackLayout {
                id: item_stack
                anchors.fill: parent
                currentIndex: m_bar.currentIndex

                BaseView {
                    type: QNCM.enums.IdTypeSong
                    delegate: QA.SongDelegate {
                        width: ListView.view.contentWidth
                        dgModel: model.data
                        onClicked: {
                            QA.Action.play_by_id(dgModel.itemId);
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
                            source: QA.Util.image_url(model.data.picUrl)
                            sourceSize.height: 48
                            sourceSize.width: 48
                        }
                        onClicked: {
                            QA.Action.route_by_id(model.data.itemId);
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
                            source: QA.Util.image_url(model.data.picUrl)
                            sourceSize.height: 48
                            sourceSize.width: 48
                        }
                        onClicked: {
                            QA.Action.route_by_id(model.data.itemId);
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
                            source: QA.Util.image_url(model.data.picUrl)
                            sourceSize.height: 48
                            sourceSize.width: 48
                        }
                        onClicked: {
                            QA.Action.route_by_id(model.data.itemId);
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
        expand: true
        leftMargin: 24
        rightMargin: 24

        topMargin: 8
        bottomMargin: MD.MatProp.size.verticalPadding + m_view_pane.bottomMargin

        property alias type: querier.type

        QNCM.PlayRecordQuerier {
            id: querier
        }
    }
}
