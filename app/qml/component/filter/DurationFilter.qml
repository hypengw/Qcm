import QtQuick
import Qcm.App as QA
import Qcm.Msg as QM

QA.IntFilter {
    name: qsTr('duration')

    required property var filter
    property QM.durationFilter subfilter
    function doCommit() {
        toFilter(subfilter);
        filter.durationFilter = subfilter;
    }
    Component.onCompleted: {
        if (!filter.hasDurationFilter) {
            filter.durationFilter = subfilter;
        }
        fromFilter(filter.durationFilter);
        commit.connect(doCommit);
    }
}
