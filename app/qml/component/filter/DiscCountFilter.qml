import QtQuick
import Qcm.App as QA
import Qcm.Msg as QM

QA.IntFilter {
    name: qsTr('disc')

    required property var filter
    property QM.discCountFilter subfilter
    function doCommit() {
        toFilter(subfilter);
        filter.discCountFilter = subfilter;
    }
    Component.onCompleted: {
        if (!filter.hasDiscCountFilter) {
            filter.discCountFilter = subfilter;
        }
        fromFilter(filter.discCountFilter);
        commit.connect(doCommit);
    }
}
