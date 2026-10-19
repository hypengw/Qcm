pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

ColumnLayout {
    id: root
    property var fields: []
    property var originalValues: ({})
    property var values: ({})
    property var fieldErrors: ({})
    readonly property var sections: {
        const result = [];
        for (const field of fields) {
            let section = result[result.length - 1];
            if (!section || section.title !== field.group) {
                section = {
                    title: field.group,
                    fields: []
                };
                result.push(section);
            }
            section.fields.push(field);
        }
        return result;
    }
    readonly property bool modified: {
        const keys = Object.keys(values);
        return keys.length !== Object.keys(originalValues).length || keys.some(key => values[key] !== originalValues[key]);
    }
    spacing: 8
    visible: fields.length > 0
    MD.FontMetrics {
        id: descriptionMetrics
        typescale: MD.Token.typescale.body_medium
    }
    onOriginalValuesChanged: reset()
    function reset() {
        values = JSON.parse(JSON.stringify(originalValues));
        fieldErrors = ({});
    }
    function setValue(key, value) {
        const next = JSON.parse(JSON.stringify(values));
        next[key] = value;
        values = next;
        const errors = Object.assign({}, fieldErrors);
        delete errors[key];
        fieldErrors = errors;
    }
    function fieldEnabled(field) {
        const condition = field.enabled_when;
        return !condition || values[condition.key] === condition.value;
    }
    Repeater {
        model: root.sections
        delegate: QA.SettingSection {
            id: section
            required property var modelData
            Layout.fillWidth: true
            title: modelData.title
            spacing: 2
            Repeater {
                model: section.modelData.fields
                onItemAdded: section.columnChanged()
                onItemRemoved: section.columnChanged()
                delegate: QA.SettingRow {
                    id: settingRow
                    required property var modelData
                    objectName: "providerSetting/" + modelData.key
                    Layout.fillWidth: true
                    readonly property bool descriptionBelow: descriptionMetrics.advanceWidth(modelData.description) > settingRow.availableWidth / 2
                    readonly property string fieldError: root.fieldErrors[modelData.key] ?? ""
                    text: modelData.name
                    supportText: descriptionBelow ? "" : modelData.description
                    below: descriptionBelow || fieldError.length > 0 ? descriptionComponent : null
                    enabled: root.fieldEnabled(modelData)
                    Component {
                        id: descriptionComponent
                        ColumnLayout {
                            spacing: 4
                            MD.Label {
                                Layout.fillWidth: true
                                visible: settingRow.descriptionBelow
                                text: settingRow.modelData.description
                                typescale: MD.Token.typescale.body_medium
                                font.capitalization: Font.MixedCase
                                color: settingRow.mdState.supportTextColor
                                wrapMode: Text.Wrap
                            }
                            MD.Label {
                                Layout.fillWidth: true
                                visible: text.length > 0
                                text: settingRow.fieldError
                                color: MD.Token.color.error
                                wrapMode: Text.Wrap
                            }
                        }
                    }
                    trailing: Loader {
                        id: control
                        Connections {
                            target: settingRow
                            function onClicked() {
                                if (settingRow.modelData.kind === "boolean")
                                    root.setValue(settingRow.modelData.key, !root.values[settingRow.modelData.key]);
                                else if (settingRow.modelData.kind === "choice")
                                    control.item?.openPopup();
                                else
                                    control.item?.forceActiveFocus();
                            }
                        }
                        sourceComponent: settingRow.modelData.kind === "choice" ? choiceEditor : settingRow.modelData.kind === "boolean" ? booleanEditor : textEditor
                        Component {
                            id: choiceEditor
                            MD.ComboBox {
                                implicitWidth: 190
                                model: settingRow.modelData.options
                                textRole: "name"
                                valueRole: "value"
                                currentIndex: {
                                    const options = settingRow.modelData.options;
                                    for (let i = 0; i < options.length; ++i)
                                        if (options[i].value === root.values[settingRow.modelData.key])
                                            return i;
                                    return -1;
                                }
                                onSelectionChangeRequested: (index, value) => root.setValue(settingRow.modelData.key, value)
                            }
                        }
                        Component {
                            id: booleanEditor
                            MD.Switch {
                                checked: root.values[settingRow.modelData.key] === true
                                onCheckedChangeRequested: checked => root.setValue(settingRow.modelData.key, checked)
                            }
                        }
                        Component {
                            id: textEditor
                            MD.TextField {
                                implicitWidth: 190
                                text: String(root.values[settingRow.modelData.key] ?? "")
                                inputMethodHints: settingRow.modelData.kind === "integer" ? Qt.ImhFormattedNumbersOnly : Qt.ImhNone
                                onTextEdited: {
                                    const n = Number(text);
                                    root.setValue(settingRow.modelData.key, settingRow.modelData.kind === "integer" && text.length > 0 && Number.isSafeInteger(n) ? n : text);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
