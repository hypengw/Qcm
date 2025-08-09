import QtQuick
import Qcm.App as QA
import Qcm.Material as MD
import Qcm.Msg as QM

QA.BaseFilter {
    id: root
    property int value

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
            name: qsTr('equal'),
            value: QM.IntCondition.INT_CONDITION_EQUAL
        },
        {
            name: qsTr('not equal'),
            value: QM.IntCondition.INT_CONDITION_EQUAL_NOT
        },
        {
            name: qsTr('less'),
            value: QM.IntCondition.INT_CONDITION_LESS
        },
        {
            name: qsTr('less equal'),
            value: QM.IntCondition.INT_CONDITION_LESS_EQUAL
        },
        {
            name: qsTr('greater'),
            value: QM.IntCondition.INT_CONDITION_GREATER
        },
        {
            name: qsTr('greater equal'),
            value: QM.IntCondition.INT_CONDITION_GREATER_EQUAL
        },
        {
            name: qsTr('any'),
            value: QM.IntCondition.INT_CONDITION_UNSPECIFIED
        }
    ]

    MD.InputChip {
        visible: root.condition !== QM.IntCondition.INT_CONDITION_UNSPECIFIED
        text: root.value
    }
}
