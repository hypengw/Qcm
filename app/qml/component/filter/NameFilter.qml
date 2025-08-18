import QtQuick
import Qcm.App as QA
import Qcm.Msg as QM

QA.StringFilter {
    name: qsTr('name')
    required property var filter
    property QM.nameFilter subfilter
    function doCommit() {
        toFilter(subfilter);
        filter.nameFilter = subfilter;
    }
    Component.onCompleted: {
        if (!filter.hasNameFilter) {
            filter.nameFilter = subfilter;
        }
        fromFilter(filter.nameFilter);
        commit.connect(doCommit);
    }
}
