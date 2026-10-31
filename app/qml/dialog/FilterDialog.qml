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

        MD.VerticalLazyList {
            id: filterView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: -16
            Layout.rightMargin: -16

            source: QA.ItemModelSource {
                model: root.model
            }
            delegate: Column {
                id: ruleRow
                required property MD.LazyRow row
                readonly property bool firstInGroup: {
                    void row.revision;
                    return row.index === 0 || filterView.source.valueAt(row.index - 1)?.group !== row.value.group;
                }
                Loader {
                    width: parent.width
                    active: ruleRow.firstInGroup
                    sourceComponent: m_section_dg
                    readonly property int groupId: ruleRow.row.value.group
                }
                Loader {
                    width: parent.width
                    sourceComponent: root.model instanceof QA.AlbumFilterRuleModel ? albumRule
                        : root.model instanceof QA.ArtistFilterRuleModel ? artistRule : mixRule
                }
                Component {
                    id: albumRule
                    QA.AlbumFilter {
                        row: ruleRow.row
                        ruleModel: root.model
                        itemSource: filterView.source
                    }
                }
                Component {
                    id: artistRule
                    QA.ArtistFilter {
                        row: ruleRow.row
                        ruleModel: root.model
                        itemSource: filterView.source
                    }
                }
                Component {
                    id: mixRule
                    QA.MixFilter {
                        row: ruleRow.row
                        ruleModel: root.model
                        itemSource: filterView.source
                    }
                }
            }
            implicitHeight: contentHeight
            spacing: 2
            leftMargin: 16
            rightMargin: 16
        }
    }

    Component {
        id: m_section_dg
        RowLayout {
            id: m_section
            spacing: 8
            readonly property int groupId: parent.groupId
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
                    onCheckedChangeRequested: checked => {
                        if (checked)
                            root.model.setLogicOpAt(m_section.sectionIndex, QM.LogicOp.LOGIC_OP_AND);
                    }
                }
                MD.SegmentedButton {
                    size: MD.Enum.XS
                    text: qsTr('OR')
                    checked: m_section.currentOp === QM.LogicOp.LOGIC_OP_OR
                    onCheckedChangeRequested: checked => {
                        if (checked)
                            root.model.setLogicOpAt(m_section.sectionIndex, QM.LogicOp.LOGIC_OP_OR);
                    }
                }
            }

            MD.StandardIconButton {
                icon.width: 22
                icon.height: 22
                implicitBackgroundSize: 0
                icon.name: MD.Token.icon.add
                onClicked: root.model.appendRuleInGroup(m_section.groupId)
            }
            MD.StandardIconButton {
                icon.width: 22
                icon.height: 22
                implicitBackgroundSize: 0
                icon.name: MD.Token.icon.delete
                onClicked: root.model.deleteGroup(m_section.groupId)
            }
        }
    }

}
