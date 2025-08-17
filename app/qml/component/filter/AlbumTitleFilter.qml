import QtQuick
import Qcm.App as QA
import Qcm.Msg as QM

QA.StringFilter {
    name: qsTr('album albumTitle')
    required property var filter
    property QM.albumTitleFilter subfilter
    function doCommit() {
        toFilter(subfilter);
        filter.albumTitleFilter = subfilter;
    }
    Component.onCompleted: {
        if (!filter.hasAlbumTitleFilter) {
            filter.albumTitleFilter = subfilter;
        }
        fromFilter(filter.albumTitleFilter);
        commit.connect(doCommit);
    }
}
