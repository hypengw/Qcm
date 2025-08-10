import QtQuick
import Qcm.App as QA
import Qcm.Msg as QM

QA.StringFilter {
    name: qsTr('title')
    required property var filter
    property QM.titleFilter subfilter
    function doCommit() {
        toFilter(subfilter);
        filter.titleFilter = subfilter;
    }
    Component.onCompleted: {
        if (!filter.hasTitleFilter) {
            filter.titleFilter = subfilter;
        }
        fromFilter(filter.titleFilter);
        commit.connect(doCommit);
    }
}
