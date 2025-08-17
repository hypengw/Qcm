import QtQuick
import Qcm.App as QA
import Qcm.Msg as QM

QA.IntFilter {
    name: qsTr('year')

    required property var filter
    property QM.yearFilter subfilter
    function doCommit() {
        toFilter(subfilter);
        filter.yearFilter = subfilter;
    }
    Component.onCompleted: {
        if (!filter.hasYearFilter) {
            filter.yearFilter = subfilter;
        }
        fromFilter(filter.yearFilter);
        commit.connect(doCommit);
    }
}
