import QtQuick
import Qcm.App as QA
import Qcm.Material as MD
import Qcm.Msg as QM

QA.BaseFilter {
    id: root
    property string value
    onValueChanged: filterChanged()

    MD.InputChip {
        text: "title"
        onClicked: root.clicked()
    }
    MD.InputChip {
        text: {
            switch (root.condition) {
            case QM.StringCondition.STRING_CONDITION_CONTAINS_NOT:
                return qsTr('not contains');
            case QM.StringCondition.STRING_CONDITION_CONTAINS:
                return qsTr('contains');
            case QM.StringCondition.STRING_CONDITION_IS:
                return qsTr('is');
            case QM.StringCondition.STRING_CONDITION_IS_NOT:
                return qsTr('is not');
            default:
                return qsTr('is');
            }
        }
    }
    MD.InputChip {
        text: root.value
    }
}
