pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qcm.App as QA
import Qcm.Material as MD
import Qcm.Msg as QM

MD.Dialog {
    id: root
    title: qsTr('filter')
    property QA.FilterRuleModel model
    horizontalPadding: 16
    implicitWidth: Math.min(420, parent ? parent.width - 48 : 420)

    standardButtons: T.Dialog.Apply | T.Dialog.Reset
    onApplied: {
        model.apply();
        accept();
    }
    onReset: model.reset()

    Component.onCompleted: {
        let btn = standardButton(T.Dialog.Reset);
        btn.enabled = Qt.binding(() => model.dirty);
        btn = standardButton(T.Dialog.Apply);
        btn.enabled = Qt.binding(() => model.dirty);
    }

    contentItem: ColumnLayout {
        RowLayout {
            MD.Label {
                Layout.fillWidth: true
                text: qsTr('Rule')
                typescale: MD.Token.typescale.title_medium
            }
            Row {
                spacing: 0
                MD.IconButton {
                    icon.name: MD.Token.icon.clear_all
                    onClicked: {
                        root.model.removeRows(0, root.model.rowCount());
                    }
                }
                MD.IconButton {
                    icon.name: MD.Token.icon.add
                    onClicked: root.model.appendNewGroup()
                }
            }
        }

        MD.VerticalListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: -16
            Layout.rightMargin: -16

            model: root.model
            delegate: {
                if (root.model instanceof QA.AlbumFilterRuleModel) {
                    return m_dg_album;
                } else if (root.model instanceof QA.ArtistFilterRuleModel) {
                    return m_dg_artist;
                } else if (root.model instanceof QA.MixFilterRuleModel) {
                    return m_dg_mix;
                }
                return null;
            }
            implicitHeight: contentHeight
            spacing: 2
            leftMargin: 16
            rightMargin: 16

            section.property: "group"
            section.criteria: ViewSection.FullString
            section.delegate: m_section_dg
        }
    }

    Component {
        id: m_section_dg
        RowLayout {
            id: m_section
            width: ListView.view.contentWidth
            spacing: 8
            required property string section
            readonly property int groupId: parseInt(m_section.section)
            readonly property int sectionIndex: root.model
                ? root.model.sectionIndexForGroup(m_section.groupId) : -1
            readonly property int currentOp: root.model && m_section.sectionIndex > 0
                ? root.model.logicOpAt(m_section.sectionIndex) : -1

            MD.Label {
                Layout.fillWidth: true
                text: qsTr('Group %1').arg(m_section.sectionIndex + 1)
                typescale: MD.Token.typescale.label_medium
            }

            MD.SegmentedButtonGroup {
                visible: m_section.sectionIndex > 0
                MD.SegmentedButton {
                    size: MD.Enum.XS
                    text: qsTr('AND')
                    checked: m_section.currentOp !== QM.LogicOp.LOGIC_OP_OR
                    onClicked: root.model.setLogicOpAt(m_section.sectionIndex,
                        QM.LogicOp.LOGIC_OP_AND)
                }
                MD.SegmentedButton {
                    size: MD.Enum.XS
                    text: qsTr('OR')
                    checked: m_section.currentOp === QM.LogicOp.LOGIC_OP_OR
                    onClicked: root.model.setLogicOpAt(m_section.sectionIndex,
                        QM.LogicOp.LOGIC_OP_OR)
                }
            }

            MD.SmallIconButton {
                icon.name: MD.Token.icon.add
                onClicked: root.model.appendRuleInGroup(m_section.groupId)
            }
            MD.SmallIconButton {
                icon.name: MD.Token.icon.delete
                onClicked: root.model.deleteGroup(m_section.groupId)
            }
        }
    }

    Component {
        id: m_dg_album
        QA.AlbumFilter {
            width: ListView.view.contentWidth
        }
    }

    Component {
        id: m_dg_artist
        QA.ArtistFilter {
            width: ListView.view.contentWidth
        }
    }

    Component {
        id: m_dg_mix
        QA.MixFilter {
            width: ListView.view.contentWidth
        }
    }
}
