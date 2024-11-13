import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Service.Ncm as QNCM
import Qcm.Material as MD

MD.Page {
    id: root
    padding: 0
    title: qsTr("daily song")
    property int vpadding: header.visible ? 0 : root.MD.MatProp.size.verticalPadding + m_view_pane.topMargin

    required property QNCM.RecommendSongsQuery query

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
            topMargin: 0
            bottomMargin: MD.MatProp.size.verticalPadding
        }

        MD.ListView {
            id: m_view
            anchors.fill: parent
            expand: true

            leftMargin: 16
            rightMargin: 16
            topMargin: MD.MatProp.size.verticalPadding
            bottomMargin: MD.MatProp.size.verticalPadding * 2

            busy: root.query.status === QA.enums.Querying
            model: root.query.data

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
                                    return root.query.data.items();
                                }
                            }
                        }
                    }
                }
            }

            delegate: QA.SongDelegate {
                required property int index
                required property var model
                width: ListView.view.contentWidth
                showCover: true
                onClicked: {
                    QA.Action.play_by_id(dgModel.itemId);
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
                        const songs = root.query.data.items();
                        QA.Action.switch_to(songs);
                    }
                }
            }
        }
    }
}
