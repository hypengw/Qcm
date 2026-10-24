import QtQuick

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    id: root
    icon.name: MD.Token.icon.album
    text: qsTr('go to album')

    property QA.item_id albumId
    enabled: root.albumId.valid
    onTriggered: {
        QA.Action.routeItem(root.albumId);
    }
}
