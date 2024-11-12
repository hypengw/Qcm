import QtQuick
import QtQuick.Controls.Basic

import Qcm.App as QA
import Qcm.Material as MD

Action {
    id: root
    icon.name: MD.Token.icon.album
    text: qsTr('go to album')

    required property QA.t_id albumId
    enabled: albumId.valid()
    onTriggered: {
        console.error(root.albumId);
        QA.Global.route(root.albumId);
    }
}
