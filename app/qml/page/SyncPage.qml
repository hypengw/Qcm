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
            width: ListView.view.contentWidth
            padding: 12

            contentItem: ColumnLayout {
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
            background: MD.Rectangle {
                color: MD.Token.color.surface_container
                radius: MD.Token.shape.corner.medium
            }
        }
    }
}
