import QtQuick
import Qcm.App as QA
import Qcm.Msg as QM

QA.StringFilter {
    name: qsTr('artist name')

    required property var filter
    property QM.artistNameFilter subfilter
    function doCommit() {
        toFilter(subfilter);
        filter.artistNameFilter = subfilter;
    }
    Component.onCompleted: {
        if (!filter.hasArtistNameFilter) {
            filter.artistNameFilter = subfilter;
        }
        fromFilter(filter.artistNameFilter);
        commit.connect(doCommit);
    }
}
