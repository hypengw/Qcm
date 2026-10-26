pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD
import Qcm.Msg as QM

MD.Page {
    id: root

    readonly property var mix: qr_pl.data.mix
    property alias itemId: qr_pl.itemId

    title: qsTr("Mix")
    padding: 0
    scrolling: !m_view.atYBeginning

    MD.FlickablePane {
        id: m_view_pane
        view: m_view
        excludeBegin: m_view.headerItem.height - m_control_pane.height + view.topMargin
        radius: root.radius
        includeHorizontalMargins: false
        bottomMargin: root.MD.MProp.page.bottomMargin
    }

    MD.PageVerticalListView {
        id: m_view
        anchors.fill: parent
        reuseItems: true
        contentY: 0

        extraBottomMargin: 16

        model: qr_pl.data
        readonly property bool single: width < m_cover.displaySize.width * (1.0 + 1.5) + 8
        readonly property bool canDelete: false

        Item {
            visible: false

            QA.Image {
                id: m_cover
                elevation: MD.Token.elevation.level2
                source: QA.Util.image_url(root.itemId)
                radius: 16

                Layout.preferredWidth: displaySize.width
                Layout.preferredHeight: displaySize.height
                displaySize: Qt.size(240, 240)
            }
            MD.Text {
                id: m_title
                Layout.fillWidth: true
                maximumLineCount: 2
                text: root.mix.name
                typescale: m_view.single ? MD.Token.typescale.headline_medium : MD.Token.typescale.headline_large
            }

            RowLayout {
                id: m_info
                spacing: 12
                MD.Text {
                    typescale: MD.Token.typescale.body_medium
                    text: `${root.mix.trackCount} songs`
                }
                MD.Text {
                    visible: root.mix.hasUpdateAt
                    typescale: MD.Token.typescale.body_medium
                    text: root.mix.hasUpdateAt ? qsTr("Locally updated on %1").arg(QA.Util.formatDateTime(root.mix.updateAt, 'yyyy.MM.dd')) : ""
                }
            }
            QA.ListDescription {
                id: m_desc
                description: root.mix.description.trim()
            }

            RowLayout {
                id: m_control_pane
                Layout.alignment: Qt.AlignHCenter

                MD.ActionIconButton {
                    hoverEnabled: true
                    MD.ToolTip.text: action?.tooltip || text
                    MD.ToolTip.visible: mdState.interactions.hovered && !mdState.interactions.pressed && MD.ToolTip.text.length > 0
                    action: QA.AppendListAction {
                        getSongIds: function () {
                            return QA.Util.collect_ids(qr_pl.data);
                        }
                    }
                }
                MD.BusyIconButton {
                    text: qsTr("Refresh")
                    hoverEnabled: true
                    MD.ToolTip.text: text
                    MD.ToolTip.visible: mdState.interactions.hovered && !mdState.interactions.pressed
                    icon.name: MD.Token.icon.refresh
                    enabled: root.itemId.valid && !qr_pl.querying
                    busy: qr_pl.querying
                    visible: root.mix.mixType !== QM.MixType.MIX_TYPE_NORMAL
                    onClicked: qr_pl.refresh()
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
            width: ListView.view.contentWidth
            leftMargin: 16
            rightMargin: 16
            canDelete: ListView.view.canDelete
            useTracknumber: false

            onClicked: {
                QA.Action.play(itemId);
            }
        }
        footer: MD.ListBusyFooter {
            running: qr_pl.status === QA.Enum.Querying
            width: ListView.view.contentWidth
        }
    }
    MD.FAB {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: root.MD.MProp.page.rightMargin + 16
        anchors.bottomMargin: root.MD.MProp.page.bottomMargin + 16
        flickable: m_view
        icon.name: MD.Token.icon.play_arrow
        onClicked: {
            const songs = QA.Util.collect_ids(qr_pl.data);
            QA.Action.switch_songs(songs);
        }
    }

    QA.MixSongsQuery {
        id: qr_pl
        Component.onCompleted: delayReload()
    }
}
