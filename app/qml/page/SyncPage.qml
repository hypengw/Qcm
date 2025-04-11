import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD
import Qcm.App as QA

MD.Page {
    id: root
    font.capitalization: Font.Capitalize
    title: qsTr('provider')
    bottomPadding: radius
    scrolling: !m_view.atYBeginning

    MD.VerticalListView {
        id: m_view
        width: root.width
        leftMargin: 12
        rightMargin: 12
        topMargin: 8
        bottomMargin: 8
        expand: true

        model: QA.App.providerStatus

        delegate: MD.Control {
            id: m_item
            required property int index
            required property var model
            readonly property var libraries: model.librariesData
            readonly property bool syncing: model.syncStatus.state == 1

            width: ListView.view.contentWidth
            topPadding: 0
            bottomPadding: 12

            QA.SyncQuery {
                id: m_query
            }

            contentItem: Column {
                spacing: 0
                MD.Control {
                    height: syncing ? implicitHeight : 0
                    width: parent.width
                    verticalPadding: 4
                    horizontalPadding: MD.Token.shape.corner.medium
                    Behavior on height {
                        NumberAnimation {
                            duration: 300
                        }
                    }

                    contentItem: MD.Label {
                        text: {
                            const s = model.syncStatus;
                            return `album: ${s.album} artist: ${s.artist} song: ${s.song}`;
                        }
                        color: MD.MProp.color.on_primary
                        typescale: MD.Token.typescale.label_medium
                    }

                    background: MD.Rectangle {
                        color: MD.Token.color.primary
                        corners: MD.Util.corner(MD.Token.shape.corner.medium, MD.Token.shape.corner.medium, 0, 0)
                    }
                }

                MD.Space {
                    spacing: syncing ? 8 : 12
                }

                ColumnLayout {
                    anchors {
                        left: parent.left
                        right: parent.right
                        leftMargin: 12
                        rightMargin: 12
                    }

                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        MD.IconSvg {
                            sourceData: QA.App.providerStatus.svg(index)
                            size: 24
                        }
                        MD.Text {
                            text: model.name
                            typescale: MD.Token.typescale.title_medium
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                        MD.Button {
                            type: MD.Enum.BtOutlined
                            text: qsTr('sync')
                            onClicked: {
                                m_query.providerId = model.itemId;
                                m_query.reload();
                            }
                        }
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: 8
                        Repeater {
                            model: m_item.libraries
                            MD.FilterChip {
                                text: modelData.name
                            }
                        }
                    }
                }
            }
            background: MD.Rectangle {
                color: MD.Token.color.surface_container
                radius: MD.Token.shape.corner.medium
            }
        }
    }
}
