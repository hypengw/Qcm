pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD
import "qrc:/Qcm/App/qml/js/util.mjs" as Util

MD.Page {
    id: root
    padding: 0
    title: qsTr("today")
    scrolling: !m_fk.atYBeginning

    MD.Flickable {
        id: m_fk
        anchors.fill: parent
        topMargin: root.MD.MatProp.size.verticalPadding
        bottomMargin: root.MD.MatProp.size.verticalPadding

        ColumnLayout {
            height: implicitHeight
            width: parent.width
            spacing: 24

            ColumnLayout {
                MD.Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    font.capitalization: Font.Capitalize
                    typescale: MD.Token.typescale.title_large
                    text: qsTr('recommended mix')
                }

                MD.Pane {
                    id: m_rmd_mix_pane
                    Layout.fillWidth: true
                    radius: root.radius
                    verticalPadding: radius

                    ColumnLayout {
                        width: parent.width
                        spacing: 4

                        QA.GridView {
                            Layout.fillWidth: true
                            fixedCellWidth: QA.Util.dynCardWidth(widthNoMargin, spacing)
                            implicitHeight: maxImplicitCellHeight
                            maxImplicitCellHeight: 200
                            model: qr_rmd_res.data.dailyPlaylists
                            flow: GridView.FlowTopToBottom
                            hookWheel: false
                            enabledCalMaxCellHeight: true

                            delegate: Item {
                                required property var modelData
                                required property int index
                                width: GridView.view.cellWidth
                                height: GridView.view.cellHeight
                                implicitHeight: children[0].implicitHeight
                                Component.onCompleted: GridView.view.calMaxCellHeight()

                                QA.PicGridDelegate {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    width: parent.GridView.view.fixedCellWidth
                                    picWidth: parent.GridView.view.fixedCellWidth
                                    image.source: QA.Util.image_url(parent.modelData.picUrl)
                                    text: parent.modelData.name

                                    onClicked: {
                                        QA.Global.route(parent.modelData.itemId);
                                    }
                                }
                            }
                            footer: MD.ListBusyFooter {
                                running: qr_rmd_res.status === QA.enums.Querying
                                width: GridView.view.count ? implicitWidth : GridView.view.width
                            }
                        }
                    }
                }
            }
            ColumnLayout {
                MD.Label {
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    font.capitalization: Font.Capitalize
                    typescale: MD.Token.typescale.title_large
                    text: qsTr('radar mix')
                }
                MD.Pane {
                    Layout.fillWidth: true
                    radius: root.radius
                    verticalPadding: radius

                    ColumnLayout {
                        width: parent.width
                        spacing: 4

                        QA.GridView {
                            Layout.fillWidth: true

                            fixedCellWidth: QA.Util.dynCardWidth(widthNoMargin, spacing)
                            implicitHeight: maxImplicitCellHeight
                            maxImplicitCellHeight: 200
                            flow: GridView.FlowTopToBottom
                            model: QNcm.RadarPlaylistIdModel {}
                            enabledCalMaxCellHeight: true
                            hookWheel: false

                            delegate: Item {
                                required property var model
                                width: GridView.view.cellWidth
                                height: GridView.view.cellHeight
                                implicitHeight: children[0].implicitHeight
                                Component.onCompleted: GridView.view.calMaxCellHeight()

                                QA.PicGridDelegate {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    picWidth: parent.GridView.view.fixedCellWidth
                                    width: picWidth
                                    image.source: QA.Util.image_url(pl_querier.data.info.picUrl)
                                    text: pl_querier.data.info.name

                                    onClicked: {
                                        QA.Global.route(parent.model.id);
                                    }
                                }

                                QA.MixDetailQuery {
                                    id: pl_querier
                                    itemId: model.id
                                }
                            }
                        }
                    }
                }
            }
            /*
            MD.Pane {
                Layout.fillWidth: true
                radius: root.radius
                verticalPadding: radius

                ColumnLayout {
                    width: parent.width
                    spacing: 4

                    MD.Text {
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        font.capitalization: Font.Capitalize
                        typescale: MD.Token.typescale.title_large
                        text: qsTr('recommended songs')
                    }
                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        MD.IconButton {
                            action: QA.AppendListAction {
                                getSongs: function () {
                                    return qr_rmd_songs.data.dailySongs;
                                }
                            }
                        }
                    }

                    MD.ListView {
                        id: view
                        Layout.fillWidth: true
                        interactive: false
                        expand: true
                        leftMargin: 16
                        rightMargin: 16

                        model: qr_rmd_songs.data.dailySongs

                        delegate: QA.SongDelegate {
                            required property int index
                            required property var modelData
                            width: ListView.view.contentWidth
                            showCover: true
                            onClicked: {
                                QA.Action.play_by_id(dgModel.itemId);
                            }
                        }
                        footer: MD.ListBusyFooter {
                            running: qr_rmd_songs.status === QA.enums.Querying
                            width: ListView.view.contentWidth
                        }
                    }
                }
            }
            */
            QNcm.RecommendSongsQuerier {
                id: qr_rmd_songs
            }
            QNcm.RecommendResourceQuerier {
                id: qr_rmd_res
            }
            // avoid loading with switch page
            Timer {
                id: timer_refresh_delay

                property bool dirty: false
                interval: 3 * 1000
                repeat: false
                running: false
                onTriggered: {
                    if (root.visible && dirty) {
                        qr_rmd_res.query();
                        qr_rmd_songs.query();
                        dirty = false;
                    }
                }
            }
            Connections {
                target: root
                function onVisibleChanged() {
                    timer_refresh_delay.start();
                }
            }
            Timer {
                id: timer_refresh

                interval: 15 * 60 * 1000
                repeat: true
                running: true

                onTriggered: {
                    timer_refresh_delay.dirty = true;
                    timer_refresh_delay.start();
                }
            }
        }
    }

    MD.FAB {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 16
        anchors.bottomMargin: 16
        flickable: m_fk
        action: Action {
            icon.name: MD.Token.icon.play_arrow

            onTriggered: {
                const songs = qr_rmd_songs.data.dailySongs.filter(s => {
                    return s.canPlay;
                });
                if (songs.length)
                    QA.App.playqueue.switchList(songs);
            }
        }
    }
}
