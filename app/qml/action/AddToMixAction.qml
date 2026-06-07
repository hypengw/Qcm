import QtQuick

import Qcm.App as QA
import Qcm.Material as MD
import Qcm.Msg as QM

MD.Action {
    id: root
    icon.name: MD.Token.icon.queue
    text: qsTr('add to mix')

    property int oper: QM.MixManipulateOper.MIX_MANIPULATE_OPER_ADD_SONGS
    property list<QA.item_id> songIds
    property list<QA.item_id> albumIds

    onTriggered: {
        const msg = QA.Util.routeMsg();
        msg.dst = 'Qcm.App/AddToMixDialog';
        msg.props = {
            oper: root.oper,
            songIds: root.songIds,
            albumIds: root.albumIds
        };
        QA.Action.openPopup(msg);
    }
}
