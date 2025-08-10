import QtQuick
import Qcm.App as QA
import Qcm.Msg as QM

QA.IntFilter {
    name: qsTr('track')

    required property var filter
    property QM.trackCountFilter subfilter
    function doCommit() {
        toFilter(subfilter);
        filter.trackFilter = subfilter;
    }
    Component.onCompleted: {
        if (!filter.hasTrackFilter) {
            filter.trackFilter = subfilter;
        }
        fromFilter(filter.trackFilter);
        commit.connect(doCommit);
    }
}
