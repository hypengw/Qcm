pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root

    property alias itemData: qr_pl.data
    property alias itemId: qr_pl.itemId

    title: qsTr("playlist")
    padding: 0
    scrolling: !m_view.atYBeginning

    MD.FlickablePane {
        id: m_view_pane
        view: m_view
        excludeBegin: m_view.headerItem.height - m_control_pane.height
        radius: root.radius
        topMargin: 0
    }

    MD.ListView {
        id: m_view
        anchors.fill: parent
        reuseItems: true
        contentY: 0

        topMargin: MD.MatProp.size.verticalPadding
        bottomMargin: MD.MatProp.size.verticalPadding + m_view_pane.bottomMargin

        model: root.itemData
        readonly property bool single: width < m_cover.displaySize.width * (1.0 + 1.5) + 8

        Item {
            visible: false

            QA.Image {
                id: m_cover
                elevation: MD.Token.elevation.level2
                source: QA.Util.image_url(root.itemData.info.picUrl)
                radius: 16

                Layout.preferredWidth: displaySize.width
                Layout.preferredHeight: displaySize.height
                displaySize: Qt.size(240, 240)
            }
            MD.Text {
                id: m_title
                Layout.fillWidth: true
                maximumLineCount: 2
                text: root.itemData.info.name
                typescale: m_view.single ? MD.Token.typescale.headline_medium : MD.Token.typescale.headline_large
            }

            RowLayout {
                id: m_info
                spacing: 12
                MD.Text {
                    typescale: MD.Token.typescale.body_medium
                    text: `${root.itemData.info.trackCount} tracks`
                }
                MD.Text {
                    typescale: MD.Token.typescale.body_medium
                    text: Qt.formatDateTime(root.itemData.info.updateTime, 'yyyy.MM.dd')
                }
            }
            QA.ListDescription {
                id: m_desc
                description: root.itemData.info.description.trim()
            }

            RowLayout {
                id: m_control_pane
                Layout.alignment: Qt.AlignHCenter
                //action: QA.AppendListAction {
                //    getSongs: function () {
                //        return itemData.songs;
                //    }
                //}
                MD.IconButton {}

                MD.IconButton {
                    id: btn_fav
                    action: QA.CollectAction {
                        itemId: root.itemId
                    }
                }
                MD.IconButton {
                    id: btn_comment
                    //action: QNcm.CommentAction {
                    //    itemId: root.itemId
                    //}
                }
            }
        }

        header: Item {
            width: parent.width
            implicitHeight: children[0].implicitHeight
            ColumnLayout {
                anchors.fill: parent
                spacing: 16

                MD.Pane {
                    Layout.fillWidth: true
                    radius: root.radius
                    padding: 16

                    ColumnLayout {
                        width: parent.width
                        RowLayout {
                            spacing: 16
                            visible: !m_view.single

                            LayoutItemProxy {
                                target: m_cover
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                                spacing: 12
                                LayoutItemProxy {
                                    Layout.fillWidth: true
                                    target: m_title
                                }
                                LayoutItemProxy {
                                    target: m_info
                                }
                                LayoutItemProxy {
                                    Layout.fillWidth: true
                                    visible: !!m_desc.description
                                    target: m_desc
                                }
                            }
                        }
                        ColumnLayout {
                            spacing: 0
                            Layout.fillWidth: true
                            visible: m_view.single

                            LayoutItemProxy {
                                Layout.alignment: Qt.AlignHCenter
                                target: m_cover
                            }

                            MD.Space {
                                spacing: 16
                            }
                            ColumnLayout {
                                Layout.alignment: Qt.AlignHCenter
                                spacing: 12

                                LayoutItemProxy {
                                    Layout.alignment: Qt.AlignHCenter
                                    Layout.maximumWidth: implicitWidth
                                    Layout.fillWidth: true
                                    target: m_title
                                }
                                LayoutItemProxy {
                                    Layout.alignment: Qt.AlignHCenter
                                    target: m_info
                                }
                                LayoutItemProxy {
                                    Layout.alignment: Qt.AlignHCenter
                                    Layout.fillWidth: true
                                    visible: !!m_desc.description
                                    target: m_desc
                                }
                            }
                        }
                    }
                }

                LayoutItemProxy {
                    target: m_control_pane
                }
            }
        }
        delegate: QA.SongDelegate {
            required property int index
            required property var modelData
            width: ListView.view.contentWidth
            leftMargin: 16
            rightMargin: 16

            onClicked: {
                QA.Action.play_by_id(dgModel.itemId);
            }
        }
        footer: MD.ListBusyFooter {
            running: qr_pl.status === QA.enums.Querying
            width: ListView.view.width
        }
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
                const songs = QA.Util.collect_ids(qr_pl.data);
                QA.Action.switch_ids(songs);
            }
        }
    }

    QA.PlaylistDetailQuery {
        id: qr_pl
    }
}
