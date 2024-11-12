pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Page {
    id: root

    property alias itemData: qr_dj.data
    property alias itemId: qr_dj.itemId

    title: qsTr("radio")
    padding: 0
    scrolling: !m_view.atYBeginning

    MD.FlickablePane {
        id: m_view_pane
        view: m_view
        excludeBegin: m_view.headerItem.height - m_control_pane.height + view.topMargin
        radius: root.radius
        bottomMargin: MD.MatProp.size.verticalPadding
    }

    MD.ListView {
        id: m_view
        anchors.fill: parent
        reuseItems: true
        contentY: 0

        topMargin: MD.MatProp.size.verticalPadding
        bottomMargin: MD.MatProp.size.verticalPadding * 2

        model: root.itemData

        readonly property bool single: width < m_cover.displaySize.width * (1.0 + 1.5) + 8

        Item {
            visible: false

            QA.Image {
                id: m_cover
                Layout.preferredWidth: displaySize.width
                Layout.preferredHeight: displaySize.height

                displaySize: Qt.size(240, 240)
                elevation: MD.Token.elevation.level2
                source: QA.Util.image_url(root.itemData.info.picUrl)
                radius: 16
            }

            MD.Text {
                id: m_title
                Layout.fillWidth: true
                maximumLineCount: 2
                text: root.itemData.info.name
                typescale: MD.Token.typescale.headline_large
            }

            RowLayout {
                id: m_info
                spacing: 12
                MD.Text {
                    typescale: MD.Token.typescale.body_medium
                    text: `${root.itemData.info.programCount} programs`
                }
                MD.Text {
                    typescale: MD.Token.typescale.body_medium
                    text: Qt.formatDateTime(root.itemData.info.createTime, 'yyyy.MM.dd')
                }
            }
            QA.ListDescription {
                id: m_desc
                description: root.itemData.info.description.trim()
            }
            RowLayout {
                id: m_control_pane
                Layout.alignment: Qt.AlignHCenter
                MD.IconButton {
                    action: QA.AppendListAction {
                        getSongs: function () {
                            return m_view.model.collectSongs();
                        }
                    }
                }
                MD.IconButton {
                    id: btn_fav
                    action: QA.CollectAction {
                        itemId: root.itemId
                    }
                }
                MD.IconButton {
                    id: btn_comment
                    visible: false
                    action: QA.CommentAction {
                        itemId: root.itemId
                    }
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
                            MD.Space {
                                spacing: 8
                            }
                        }
                    }
                }

                LayoutItemProxy {
                    target: m_control_pane
                }
            }
        }
        delegate: QA.ProgramDelegate {
            required property int index
            required property var model
            width: ListView.view.contentWidth

            leftMargin: 16
            rightMargin: 16

            dgModel: model

            onClicked: {
                QA.Action.play(m_view.model.song(index));
            }
        }
        footer: MD.ListBusyFooter {
            running: qr_dj.status === QA.enums.Querying
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
                const songs = m_view.model.collectSongs();
                console.error(songs);
                QA.Action.switch_to(songs);
            }
        }
    }

    QA.RadioDetailQuery {
        id: qr_dj
    }
}
