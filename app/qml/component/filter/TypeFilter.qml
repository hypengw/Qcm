pragma ComponentBehavior: Bound
import QtQuick
import Qcm.App as QA
import Qcm.Material as MD
import Qcm.Msg as QM

QA.BaseFilter {
    id: root
    required property var filter
    property QM.typeFilter subfilter
    property int value

    function fromFilter(filter) {
        condition = filter.condition;
        value = filter.value;
    }
    function toFilter(filter) {
        filter.condition = condition;
        filter.value = value;
    }
    function doCommit() {
        toFilter(subfilter);
        filter.typeFilter = subfilter;
    }
    Component.onCompleted: {
        if (!filter.hasTypeFilter) {
            filter.typeFilter = subfilter;
        }
        fromFilter(filter.typeFilter);
        commit.connect(doCommit);
    }

    conditionModel: [
        {
            name: qsTr('is'),
            value: QM.TypeCondition.TYPE_CONDITION_IS
        },
        {
            name: qsTr('is not'),
            value: QM.TypeCondition.TYPE_CONDITION_IS_NOT
        },
        {
            name: qsTr('any'),
            value: QM.TypeCondition.TYPE_CONDITION_UNSPECIFIED
        }
    ]

}
