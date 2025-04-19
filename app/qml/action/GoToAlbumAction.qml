import QtQuick

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    id: root
    icon.name: MD.Token.icon.album
    text: qsTr('go to album')

    property var albumId
    enabled: {
        console.error(albumId);
        return albumId.valid;
    }
    onTriggered: {
        QA.Action.route_by_id(root.albumId);
    }
}
