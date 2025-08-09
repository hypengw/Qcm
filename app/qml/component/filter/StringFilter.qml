pragma ComponentBehavior: Bound
import QtQuick
import QtQml.Models

import Qcm.App as QA
import Qcm.Material as MD
import Qcm.Msg as QM

QA.BaseFilter {
    id: root
    property alias value: m_value.text

    function fromFilter(filter) {
        condition = filter.condition;
        value = filter.value;
    }
    function toFilter(filter) {
        filter.condition = condition;
        filter.value = value;
    }

    conditionModel: [
        {
            name: qsTr('contains'),
            value: QM.StringCondition.STRING_CONDITION_CONTAINS
        },
        {
            name: qsTr('not contains'),
            value: QM.StringCondition.STRING_CONDITION_CONTAINS_NOT
        },
        {
            name: qsTr('is'),
            value: QM.StringCondition.STRING_CONDITION_IS
        },
        {
            name: qsTr('is not'),
            value: QM.StringCondition.STRING_CONDITION_IS_NOT
        },
        {
            name: qsTr('any'),
            value: QM.StringCondition.STRING_CONDITION_UNSPECIFIED
        }
    ]

    MD.InputChip {
        id: m_value
        visible: root.condition !== QM.StringCondition.STRING_CONDITION_UNSPECIFIED
        onClicked: edit = true
        editDelegate: MD.TextInput {
            text: root.value
            onAccepted: {
                root.value = text;
                m_value.edit = false;
            }
        }
    }
}
