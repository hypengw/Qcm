import QtQuick

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    id: root
    icon.name: MD.Token.icon.queue
    text: qsTr('add to mix')
    property list<QA.item_id> songIds
    onTriggered: {
        const msg = QA.Util.routeMsg();
        msg.dst = 'Qcm.App/AddToMixDialog';
        msg.props = {
            songIds: root.songIds
        };
        QA.Action.openPopup(msg);
    }
}
